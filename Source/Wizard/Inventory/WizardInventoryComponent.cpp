#include "Inventory/WizardInventoryComponent.h"
#include "WizardPlayerState.h"
#include "GameFramework/Pawn.h"
#include "Loot/WizardWorldPickup.h"
#include "Loot/WizardPickupComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GAS/WizardAttributeSet.h"
#include "Skills/WizardSkillEffects.h"
UWizardInventoryComponent::UWizardInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	State.Slots.SetNum(Capacity);
}

bool UWizardInventoryComponent::ResolveSnapshot(TArray<FWizardInventoryEntry>& Slots) const
{
	if (Slots.Num() != Capacity) return false;
	TSet<FGuid> Ids;
	TMap<FName, const UWizardItemDefinition*> Definitions;
	for (FWizardInventoryEntry& Entry : Slots)
	{
		if (Entry.IsEmpty()) continue;
		const UWizardItemDefinition* Definition = Entry.Definition ? Entry.Definition.Get() : UWizardItemDefinition::Resolve(Entry.DefinitionId);
		if (!Definition || !Definition->IsConfigurationValid() || Definition->ItemId != Entry.DefinitionId
			|| !Entry.InstanceId.IsValid() || Entry.Quantity <= 0 || Entry.Quantity > Definition->GetStackLimit() || Ids.Contains(Entry.InstanceId)) return false;
		const UWizardItemDefinition* const* Existing = Definitions.Find(Entry.DefinitionId);
		if (Existing && *Existing != Definition) return false;
		Definitions.Add(Entry.DefinitionId, Definition);
		Entry.Definition = Definition;
		Ids.Add(Entry.InstanceId);
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

EWizardInventoryResult UWizardInventoryComponent::ValidateRequest(const FWizardInventoryRequest& Request) const
{
	if (bMutating) return EWizardInventoryResult::Busy;
	if (!State.Slots.IsValidIndex(Request.Slot)) return EWizardInventoryResult::InvalidSlot;
	if (Request.Revision != State.Revision || !Request.InstanceId.IsValid() || State.Slots[Request.Slot].InstanceId != Request.InstanceId) return EWizardInventoryResult::StaleRequest;
	return EWizardInventoryResult::Success;
}

void UWizardInventoryComponent::Commit(TArray<FWizardInventoryEntry>&& Slots)
{
	if (State.Slots == Slots) return;
	State.Slots = MoveTemp(Slots);
	++State.Revision;
	OnChanged.Broadcast();
}

EWizardInventoryResult UWizardInventoryComponent::AddItems(const UWizardItemDefinition* Definition, int32 Quantity, int32& Accepted)
{
	Accepted = 0;
	if (bMutating) return EWizardInventoryResult::Busy;
	if (Quantity <= 0) return EWizardInventoryResult::InvalidQuantity;
	if (!Definition) return EWizardInventoryResult::UnknownDefinition;
	if (!Definition->IsConfigurationValid()) return EWizardInventoryResult::InvalidDefinition;
	const FName Id = Definition->ItemId;
	// 同一稳定ID不能在一个背包中代表不同定义，避免错误合并；无需预先登记目录。
	for (const FWizardInventoryEntry& Entry : State.Slots)
	{
		if (Entry.DefinitionId == Id && Entry.Definition != Definition) return EWizardInventoryResult::InvalidDefinition;
	}
	TArray<FWizardInventoryEntry> Slots = State.Slots;
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
	Commit(MoveTemp(Slots));
	return EWizardInventoryResult::Success;
}

EWizardInventoryResult UWizardInventoryComponent::Move(FWizardInventoryRequest Request, int32 TargetSlot)
{
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

EWizardInventoryResult UWizardInventoryComponent::Split(FWizardInventoryRequest Request, int32 Quantity, int32 TargetSlot)
{
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
	if (bMutating) return EWizardInventoryResult::Busy;
	TArray<FWizardInventoryEntry> Slots = Snapshot.Slots;
	if (!ResolveSnapshot(Slots)) return EWizardInventoryResult::InvalidSnapshot;
	Commit(MoveTemp(Slots));
	return EWizardInventoryResult::Success;
}

EWizardInventoryResult UWizardInventoryComponent::Use(FWizardInventoryRequest Request)
{
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
