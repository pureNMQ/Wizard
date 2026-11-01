#include "Inventory/WizardInventoryComponent.h"
#include "WizardPlayerState.h"
#include "GameFramework/Pawn.h"
#include "Loot/WizardWorldPickup.h"
#include "Loot/WizardPickupComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GAS/WizardAttributeSet.h"
#include "Skills/WizardSkillEffects.h"
#include "Trading/WizardShopCatalog.h"
UWizardInventoryComponent::UWizardInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	State.Slots.SetNum(Capacity);
	State.Equipment.SetNum(EquipmentCapacity);
}

bool UWizardInventoryComponent::ResolveSnapshot(FWizardInventorySnapshot& Snapshot) const
{
	if (Snapshot.Slots.Num() != Capacity || Snapshot.Equipment.Num() != EquipmentCapacity) return false;
	TSet<FGuid> Ids;
	TMap<FName, const UWizardItemDefinition*> Definitions;
	auto ResolveEntry = [&](FWizardInventoryEntry& Entry)
	{
		if (Entry.IsEmpty()) return true;
		const UWizardItemDefinition* Definition = Entry.Definition ? Entry.Definition.Get() : UWizardItemDefinition::Resolve(Entry.DefinitionId);
		if (!Definition || !Definition->IsConfigurationValid() || Definition->ItemId != Entry.DefinitionId
			|| !Entry.InstanceId.IsValid() || Entry.Quantity <= 0 || Entry.Quantity > Definition->GetStackLimit() || Ids.Contains(Entry.InstanceId)) return false;
		const UWizardItemDefinition* const* Existing = Definitions.Find(Entry.DefinitionId);
		if (Existing && *Existing != Definition) return false;
		Definitions.Add(Entry.DefinitionId, Definition);
		Entry.Definition = Definition;
		Ids.Add(Entry.InstanceId);
		return true;
	};
	for (FWizardInventoryEntry& Entry : Snapshot.Slots)
	{
		if (!ResolveEntry(Entry)) return false;
	}
	for (int32 Index = 0; Index < EquipmentCapacity; ++Index)
	{
		FWizardInventoryEntry& Entry = Snapshot.Equipment[Index];
		if (!ResolveEntry(Entry)) return false;
		if (!Entry.IsEmpty() && (Entry.Definition->Type != EWizardItemType::Equipment || static_cast<int32>(Entry.Definition->EquipmentSlot) != Index)) return false;
	}
	return true;
}

FWizardInventoryRequest UWizardInventoryComponent::MakeRequest(int32 Slot) const
{
	FWizardInventoryRequest Request;
	Request.Slot = Slot;
	Request.Revision = State.Revision;
	if (State.Slots.IsValidIndex(Slot)) Request.InstanceId = State.Slots[Slot].InstanceId;
	return Request;
}

FWizardInventoryRequest UWizardInventoryComponent::MakeEquipmentRequest(EWizardEquipmentSlot Slot) const
{
	FWizardInventoryRequest Request;
	Request.bEquipment = true;
	Request.Slot = static_cast<int32>(Slot);
	Request.Revision = State.Revision;
	if (State.Equipment.IsValidIndex(Request.Slot)) Request.InstanceId = State.Equipment[Request.Slot].InstanceId;
	return Request;
}

EWizardInventoryResult UWizardInventoryComponent::ValidateRequest(const FWizardInventoryRequest& Request) const
{
	if (IsMutationBlocked()) return EWizardInventoryResult::Busy;
	const TArray<FWizardInventoryEntry>& Entries = Request.bEquipment ? State.Equipment : State.Slots;
	if (!Entries.IsValidIndex(Request.Slot)) return EWizardInventoryResult::InvalidSlot;
	if (Request.Revision != State.Revision || !Request.InstanceId.IsValid() || Entries[Request.Slot].InstanceId != Request.InstanceId) return EWizardInventoryResult::StaleRequest;
	return EWizardInventoryResult::Success;
}

void UWizardInventoryComponent::Commit(TArray<FWizardInventoryEntry>&& Slots)
{
	TArray<FWizardInventoryEntry> Equipment = State.Equipment;
	Commit(MoveTemp(Slots), MoveTemp(Equipment));
}

void UWizardInventoryComponent::Commit(TArray<FWizardInventoryEntry>&& Slots, TArray<FWizardInventoryEntry>&& Equipment)
{
	if (State.Slots == Slots && State.Equipment == Equipment) return;
	// 广播期间GAS会同步通知属性；拒绝观察者重入，保持一次换装的完整视图。
	TGuardValue<bool> Guard(bMutating, true);
	State.Slots = MoveTemp(Slots);
	State.Equipment = MoveTemp(Equipment);
	++State.Revision;
	OnChanged.Broadcast();
}

EWizardInventoryResult UWizardInventoryComponent::AddItems(const UWizardItemDefinition* Definition, int32 Quantity, int32& Accepted)
{
	Accepted = 0;
	if (IsMutationBlocked()) return EWizardInventoryResult::Busy;
	TArray<FWizardInventoryEntry> Slots;
	const EWizardInventoryResult Result = PrepareAddItems(Definition, Quantity, Slots, Accepted);
	if (Result == EWizardInventoryResult::Success) Commit(MoveTemp(Slots));
	return Result;
}

bool UWizardInventoryComponent::IsMutationBlocked() const
{
	const AWizardPlayerState* Player = Cast<AWizardPlayerState>(GetOwner());
	return bMutating || (Player && Player->bChangingGold);
}

EWizardInventoryResult UWizardInventoryComponent::PrepareAddItems(const UWizardItemDefinition* Definition, int32 Quantity, TArray<FWizardInventoryEntry>& Slots, int32& Accepted) const
{
	Accepted = 0;
	if (Quantity <= 0) return EWizardInventoryResult::InvalidQuantity;
	if (!Definition) return EWizardInventoryResult::UnknownDefinition;
	if (!Definition->IsConfigurationValid()) return EWizardInventoryResult::InvalidDefinition;
	const FName Id = Definition->ItemId;
	// 同一稳定ID不能在一个背包中代表不同定义，避免错误合并；无需预先登记目录。
	for (const FWizardInventoryEntry& Entry : State.Slots)
	{
		if (Entry.DefinitionId == Id && Entry.Definition != Definition) return EWizardInventoryResult::InvalidDefinition;
	}
	for (const FWizardInventoryEntry& Entry : State.Equipment)
	{
		if (Entry.DefinitionId == Id && Entry.Definition != Definition) return EWizardInventoryResult::InvalidDefinition;
	}
	Slots = State.Slots;
	int32 Remaining = Quantity;
	for (FWizardInventoryEntry& Entry : Slots)
	{
		if (Entry.DefinitionId != Id || Entry.Quantity >= Definition->GetStackLimit()) continue;
		const int32 Amount = FMath::Min(Remaining, Definition->GetStackLimit() - Entry.Quantity);
		Entry.Quantity += Amount;
		Remaining -= Amount;
	}
	for (FWizardInventoryEntry& Entry : Slots)
	{
		if (Remaining == 0) break;
		if (!Entry.IsEmpty()) continue;
		Entry.InstanceId = FGuid::NewGuid();
		Entry.DefinitionId = Id;
		Entry.Definition = Definition;
		Entry.Quantity = FMath::Min(Remaining, Definition->GetStackLimit());
		Remaining -= Entry.Quantity;
	}
	Accepted = Quantity - Remaining;
	if (Accepted == 0) return EWizardInventoryResult::NoCapacity;
	return EWizardInventoryResult::Success;
}

EWizardInventoryResult UWizardInventoryComponent::Move(FWizardInventoryRequest Request, int32 TargetSlot)
{
	if (Request.bEquipment) return EWizardInventoryResult::InvalidSlot;
	const EWizardInventoryResult Result = ValidateRequest(Request);
	if (Result != EWizardInventoryResult::Success) return Result;
	if (!State.Slots.IsValidIndex(TargetSlot)) return EWizardInventoryResult::InvalidSlot;
	if (Request.Slot == TargetSlot) return EWizardInventoryResult::Success;
	TArray<FWizardInventoryEntry> Slots = State.Slots;
	FWizardInventoryEntry& Source = Slots[Request.Slot];
	FWizardInventoryEntry& Target = Slots[TargetSlot];
	const UWizardItemDefinition* Definition = Source.Definition.Get();
	if (!Definition || !Definition->IsConfigurationValid()) return EWizardInventoryResult::InvalidDefinition;
	if (Target.DefinitionId == Source.DefinitionId && Definition->GetStackLimit() > 1)
	{
		const int32 Amount = FMath::Min(Source.Quantity, Definition->GetStackLimit() - Target.Quantity);
		if (Amount <= 0) return EWizardInventoryResult::NoCapacity;
		Target.Quantity += Amount;
		Source.Quantity -= Amount;
		if (Source.Quantity == 0) Source = {};
	}
	else Slots.Swap(Request.Slot, TargetSlot);
	Commit(MoveTemp(Slots));
	return EWizardInventoryResult::Success;
}

EWizardInventoryResult UWizardInventoryComponent::Equip(FWizardInventoryRequest Request, EWizardEquipmentSlot Slot)
{
	const EWizardInventoryResult Result = ValidateRequest(Request);
	if (Result != EWizardInventoryResult::Success) return Result;
	const int32 TargetIndex = static_cast<int32>(Slot);
	if (!State.Equipment.IsValidIndex(TargetIndex)) return EWizardInventoryResult::InvalidSlot;
	const FWizardInventoryEntry& Source = Request.bEquipment ? State.Equipment[Request.Slot] : State.Slots[Request.Slot];
	const UWizardItemDefinition* Definition = Source.Definition.Get();
	if (!Definition || !Definition->IsConfigurationValid()) return EWizardInventoryResult::InvalidDefinition;
	if (Definition->Type != EWizardItemType::Equipment) return EWizardInventoryResult::NotEquipment;
	if (Definition->EquipmentSlot != Slot) return EWizardInventoryResult::WrongEquipmentSlot;
	if (Request.bEquipment)
	{
		// 每种装备只有一个合法槽；原地放下不产生物品变更。
		return Request.Slot == TargetIndex ? EWizardInventoryResult::Success : EWizardInventoryResult::WrongEquipmentSlot;
	}
	FWizardInventorySnapshot Next = State;
	Swap(Next.Slots[Request.Slot], Next.Equipment[TargetIndex]);
	if (!ResolveSnapshot(Next)) return EWizardInventoryResult::InvalidDefinition;
	Commit(MoveTemp(Next.Slots), MoveTemp(Next.Equipment));
	return EWizardInventoryResult::Success;
}

EWizardInventoryResult UWizardInventoryComponent::Unequip(FWizardInventoryRequest Request, int32 TargetSlot)
{
	const EWizardInventoryResult Result = ValidateRequest(Request);
	if (Result != EWizardInventoryResult::Success) return Result;
	if (!Request.bEquipment) return EWizardInventoryResult::InvalidSlot;
	if (TargetSlot == INDEX_NONE)
	{
		TargetSlot = State.Slots.IndexOfByPredicate([](const FWizardInventoryEntry& Entry) { return Entry.IsEmpty(); });
		if (TargetSlot == INDEX_NONE) return EWizardInventoryResult::NoCapacity;
	}
	if (!State.Slots.IsValidIndex(TargetSlot)) return EWizardInventoryResult::InvalidSlot;
	const FWizardInventoryEntry& Target = State.Slots[TargetSlot];
	if (!Target.IsEmpty())
	{
		const UWizardItemDefinition* Definition = Target.Definition.Get();
		if (!Definition || !Definition->IsConfigurationValid()) return EWizardInventoryResult::InvalidDefinition;
		if (Definition->Type != EWizardItemType::Equipment) return EWizardInventoryResult::NotEquipment;
		if (static_cast<int32>(Definition->EquipmentSlot) != Request.Slot) return EWizardInventoryResult::WrongEquipmentSlot;
	}
	FWizardInventorySnapshot Next = State;
	Swap(Next.Slots[TargetSlot], Next.Equipment[Request.Slot]);
	if (!ResolveSnapshot(Next)) return EWizardInventoryResult::InvalidDefinition;
	Commit(MoveTemp(Next.Slots), MoveTemp(Next.Equipment));
	return EWizardInventoryResult::Success;
}

EWizardInventoryResult UWizardInventoryComponent::Split(FWizardInventoryRequest Request, int32 Quantity, int32 TargetSlot)
{
	if (Request.bEquipment) return EWizardInventoryResult::InvalidSlot;
	const EWizardInventoryResult Result = ValidateRequest(Request);
	if (Result != EWizardInventoryResult::Success) return Result;
	if (!State.Slots.IsValidIndex(TargetSlot)) return EWizardInventoryResult::InvalidSlot;
	if (Quantity <= 0 || Quantity >= State.Slots[Request.Slot].Quantity) return EWizardInventoryResult::InvalidQuantity;
	if (!State.Slots[TargetSlot].IsEmpty()) return EWizardInventoryResult::NoCapacity;
	TArray<FWizardInventoryEntry> Slots = State.Slots;
	Slots[TargetSlot] = Slots[Request.Slot];
	Slots[TargetSlot].InstanceId = FGuid::NewGuid();
	Slots[TargetSlot].Quantity = Quantity;
	Slots[Request.Slot].Quantity -= Quantity;
	Commit(MoveTemp(Slots));
	return EWizardInventoryResult::Success;
}

EWizardInventoryResult UWizardInventoryComponent::RestoreSnapshot(const FWizardInventorySnapshot& Snapshot)
{
	if (IsMutationBlocked()) return EWizardInventoryResult::Busy;
	FWizardInventorySnapshot Resolved = Snapshot;
	if (!ResolveSnapshot(Resolved)) return EWizardInventoryResult::InvalidSnapshot;
	Commit(MoveTemp(Resolved.Slots), MoveTemp(Resolved.Equipment));
	return EWizardInventoryResult::Success;
}

EWizardInventoryResult UWizardInventoryComponent::Use(FWizardInventoryRequest Request)
{
	if (Request.bEquipment) return EWizardInventoryResult::NotConsumable;
	const EWizardInventoryResult Result = ValidateRequest(Request);
	if (Result != EWizardInventoryResult::Success) return Result;
	const UWizardItemDefinition* Definition = State.Slots[Request.Slot].Definition.Get();
	if (!Definition || !Definition->IsConfigurationValid()) return EWizardInventoryResult::InvalidDefinition;
	if (Definition->Type != EWizardItemType::Consumable || Definition->PotionResource == EWizardPotionResource::None) return EWizardInventoryResult::NotConsumable;
	IAbilitySystemInterface* OwnerInterface = Cast<IAbilitySystemInterface>(GetOwner());
	UAbilitySystemComponent* ASC = OwnerInterface ? OwnerInterface->GetAbilitySystemComponent() : nullptr;
	if (!ASC || !ASC->GetSet<UWizardAttributeSet>()) return EWizardInventoryResult::EffectFailed;
	if (ASC->GetNumericAttribute(UWizardAttributeSet::GetHealthAttribute()) <= 0.f) return EWizardInventoryResult::Dead;
	const bool bHealth = Definition->PotionResource == EWizardPotionResource::Health;
	const FGameplayAttribute Attribute = bHealth ? UWizardAttributeSet::GetHealthAttribute() : UWizardAttributeSet::GetManaAttribute();
	const FGameplayAttribute Maximum = bHealth ? UWizardAttributeSet::GetMaxHealthAttribute() : UWizardAttributeSet::GetMaxManaAttribute();
	const float Before = ASC->GetNumericAttribute(Attribute);
	const float MaximumValue = ASC->GetNumericAttribute(Maximum);
	if (!FMath::IsFinite(Before) || !FMath::IsFinite(MaximumValue)) return EWizardInventoryResult::EffectFailed;
	if (Before >= MaximumValue) return EWizardInventoryResult::FullResource;
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(bHealth ? UWizardSkillHealEffect::StaticClass() : UWizardSkillManaEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
	if (!Spec.IsValid()) return EWizardInventoryResult::EffectFailed;
	Spec.Data->SetSetByCallerMagnitude(WizardSkills::ValueTag(), Definition->PotionMagnitude);
	TArray<FWizardInventoryEntry> Slots = State.Slots;
	if (--Slots[Request.Slot].Quantity == 0) Slots[Request.Slot] = {};
	{
		// GAS同步属性通知可能回调UI/业务；锁定本次实例直到GE完成，避免重入重复服药。
		TGuardValue<bool> Guard(bMutating, true);
		ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		if (ASC->GetNumericAttribute(Attribute) <= Before) return EWizardInventoryResult::EffectFailed;
		State.Slots = MoveTemp(Slots);
		++State.Revision;
	}
	OnChanged.Broadcast();
	return EWizardInventoryResult::Success;
}

EWizardInventoryResult UWizardInventoryComponent::PrepareDrop(FWizardInventoryRequest Request, int32 Quantity, FWizardInventoryDropTicket& Ticket) const
{
	Ticket = {};
	if (Request.bEquipment) return EWizardInventoryResult::InvalidSlot;
	const EWizardInventoryResult Result = ValidateRequest(Request);
	if (Result != EWizardInventoryResult::Success) return Result;
	if (Quantity <= 0 || Quantity > State.Slots[Request.Slot].Quantity) return EWizardInventoryResult::InvalidQuantity;
	Ticket.Request = Request;
	Ticket.Entry = State.Slots[Request.Slot];
	Ticket.Entry.Quantity = Quantity;
	return EWizardInventoryResult::Success;
}

EWizardInventoryResult UWizardInventoryComponent::CommitDrop(const FWizardInventoryDropTicket& Ticket, TFunctionRef<bool(const FWizardInventoryEntry&)> Receiver)
{
	FWizardInventoryDropTicket Verified;
	const EWizardInventoryResult Result = PrepareDrop(Ticket.Request, Ticket.Entry.Quantity, Verified);
	if (Result != EWizardInventoryResult::Success) return Result;
	if (!(Ticket.Entry == Verified.Entry)) return EWizardInventoryResult::StaleRequest;
	TArray<FWizardInventoryEntry> Slots = State.Slots;
	Slots[Ticket.Request.Slot].Quantity -= Ticket.Entry.Quantity;
	if (Slots[Ticket.Request.Slot].Quantity == 0) Slots[Ticket.Request.Slot] = {};
	FWizardInventoryEntry Dropped = Ticket.Entry;
	// 交接记录不能与保留的背包栈共用身份；地面对象独占数量，回包时分配或合并到背包栈身份。
	if (!Slots[Ticket.Request.Slot].IsEmpty()) Dropped.InstanceId = FGuid::NewGuid();
	{
		TGuardValue<bool> Guard(bMutating, true);
		if (!Receiver(Dropped)) return EWizardInventoryResult::DropUnavailable;
		State.Slots = MoveTemp(Slots);
		++State.Revision;
	}
	OnChanged.Broadcast();
	return EWizardInventoryResult::Success;
}

EWizardInventoryResult UWizardInventoryComponent::Discard(FWizardInventoryRequest Request, int32 Quantity)
{
	FWizardInventoryDropTicket Ticket;
	const EWizardInventoryResult Prepared = PrepareDrop(Request, Quantity, Ticket);
	if (Prepared != EWizardInventoryResult::Success) return Prepared;
	AWizardPlayerState* Player = Cast<AWizardPlayerState>(GetOwner());
	APawn* Pawn = Player ? Player->GetPawn() : nullptr;
	if (!IsValid(Pawn))
	{
		return EWizardInventoryResult::DropUnavailable;
	}
	const UWizardItemDefinition* Definition = Ticket.Entry.Definition.Get();
	AWizardWorldPickup* Pickup = nullptr;
	const EWizardInventoryResult Result = CommitDrop(Ticket, [&](const FWizardInventoryEntry& Entry)
	{
		// 碰撞与拾取在来源扣量完成后才激活，暂停中的丢弃也不会即时捡回。
		Pickup = AWizardWorldPickup::SpawnItem(Pawn->GetWorld(), Definition, Entry.Quantity, Pawn->GetActorLocation());
		return IsValid(Pickup);
	});
	if (Result == EWizardInventoryResult::Success && IsValid(Pickup))
	{
		Player->GetPickup()->BlockUntilExit(Pickup);
		Pickup->ActivatePickup();
	}
	else if (IsValid(Pickup)) Pickup->Destroy();
	return Result;
}

EWizardInventoryResult UWizardInventoryComponent::CheckPurchase(const UWizardShopCatalog* Catalog, int32 ProductIndex, int32 Quantity, int64 ExpectedRevision) const
{
	if (IsMutationBlocked()) return EWizardInventoryResult::Busy;
	const AWizardPlayerState* Player = Cast<AWizardPlayerState>(GetOwner());
	if (!Player || !Player->HasAuthority()) return EWizardInventoryResult::ShopUnavailable;
	if (ExpectedRevision != State.Revision) return EWizardInventoryResult::StaleRequest;
	if (Quantity <= 0) return EWizardInventoryResult::InvalidQuantity;
	if (!IsValid(Catalog) || !Catalog->Products.IsValidIndex(ProductIndex)) return EWizardInventoryResult::UnknownDefinition;
	const FWizardShopProduct& Product = Catalog->Products[ProductIndex];
	if (Product.BuyPrice <= 0) return EWizardInventoryResult::InvalidPrice;
	if (!Catalog->IsConfigurationValid()) return EWizardInventoryResult::InvalidDefinition;
	const int64 Cost = static_cast<int64>(Product.BuyPrice) * Quantity;
	if (Cost > MAX_int32) return EWizardInventoryResult::GoldOverflow;
	if (Player->Gold < Cost) return EWizardInventoryResult::InsufficientGold;
	TArray<FWizardInventoryEntry> Slots;
	int32 Accepted = 0;
	const EWizardInventoryResult Result = PrepareAddItems(Product.Definition.Get(), Quantity, Slots, Accepted);
	if (Result != EWizardInventoryResult::Success) return Result;
	return Accepted == Quantity ? EWizardInventoryResult::Success : EWizardInventoryResult::NoCapacity;
}

int32 UWizardInventoryComponent::GetPurchaseLimit(const UWizardShopCatalog* Catalog, int32 ProductIndex) const
{
	const AWizardPlayerState* Player = Cast<AWizardPlayerState>(GetOwner());
	if (!Player || !Player->HasAuthority() || !IsValid(Catalog)
		|| !Catalog->Products.IsValidIndex(ProductIndex) || !Catalog->IsConfigurationValid()) return 0;
	const FWizardShopProduct& Product = Catalog->Products[ProductIndex];
	TArray<FWizardInventoryEntry> Slots;
	int32 Accepted = 0;
	if (PrepareAddItems(Product.Definition.Get(), MAX_int32, Slots, Accepted) != EWizardInventoryResult::Success) return 0;
	return FMath::Min(Accepted, Player->Gold / Product.BuyPrice);
}

EWizardInventoryResult UWizardInventoryComponent::Buy(const UWizardShopCatalog* Catalog, int32 ProductIndex, int32 Quantity, int64 ExpectedRevision)
{
	const EWizardInventoryResult Validated = CheckPurchase(Catalog, ProductIndex, Quantity, ExpectedRevision);
	if (Validated != EWizardInventoryResult::Success) return Validated;
	const FWizardShopProduct& Product = Catalog->Products[ProductIndex];
	TArray<FWizardInventoryEntry> Slots;
	int32 Accepted = 0;
	const EWizardInventoryResult Prepared = PrepareAddItems(Product.Definition.Get(), Quantity, Slots, Accepted);
	if (Prepared != EWizardInventoryResult::Success) return Prepared;
	if (Accepted != Quantity) return EWizardInventoryResult::NoCapacity;
	AWizardPlayerState* Player = CastChecked<AWizardPlayerState>(GetOwner());
	// 两份权威状态先同时落定，之后才广播；任何同步观察者看到完整交易且不能重入修改。
	TGuardValue<bool> InventoryGuard(bMutating, true);
	TGuardValue<bool> GoldGuard(Player->bChangingGold, true);
	State.Slots = MoveTemp(Slots);
	++State.Revision;
	Player->Gold -= static_cast<int32>(static_cast<int64>(Product.BuyPrice) * Quantity);
	OnChanged.Broadcast();
	Player->OnGoldChanged.Broadcast();
	return EWizardInventoryResult::Success;
}

EWizardInventoryResult UWizardInventoryComponent::Sell(FWizardInventoryRequest Request, int32 Quantity)
{
	const EWizardInventoryResult Validated = ValidateRequest(Request);
	if (Validated != EWizardInventoryResult::Success) return Validated;
	if (Request.bEquipment) return EWizardInventoryResult::NotTradable;
	AWizardPlayerState* Player = Cast<AWizardPlayerState>(GetOwner());
	if (!Player || !Player->HasAuthority()) return EWizardInventoryResult::ShopUnavailable;
	const FWizardInventoryEntry& Entry = State.Slots[Request.Slot];
	if (Quantity <= 0 || Quantity > Entry.Quantity) return EWizardInventoryResult::InvalidQuantity;
	const UWizardItemDefinition* Definition = Entry.Definition.Get();
	if (!IsValid(Definition)) return EWizardInventoryResult::UnknownDefinition;
	if (!Definition->bTradable) return EWizardInventoryResult::NotTradable;
	if (Definition->SellPrice <= 0) return EWizardInventoryResult::InvalidPrice;
	if (!Definition->IsConfigurationValid()) return EWizardInventoryResult::InvalidDefinition;
	const int64 Revenue = static_cast<int64>(Definition->SellPrice) * Quantity;
	if (Revenue > MAX_int32 || Revenue > MAX_int32 - Player->Gold) return EWizardInventoryResult::GoldOverflow;
	TArray<FWizardInventoryEntry> Slots = State.Slots;
	Slots[Request.Slot].Quantity -= Quantity;
	if (Slots[Request.Slot].Quantity == 0) Slots[Request.Slot] = {};
	TGuardValue<bool> InventoryGuard(bMutating, true);
	TGuardValue<bool> GoldGuard(Player->bChangingGold, true);
	State.Slots = MoveTemp(Slots);
	++State.Revision;
	Player->Gold += static_cast<int32>(Revenue);
	OnChanged.Broadcast();
	Player->OnGoldChanged.Broadcast();
	return EWizardInventoryResult::Success;
}
