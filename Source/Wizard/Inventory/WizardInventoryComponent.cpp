#include "Inventory/WizardInventoryComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GAS/WizardAttributeSet.h"
#include "Skills/WizardSkillEffects.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#if WITH_EDITOR
EDataValidationResult UWizardInventoryConfig::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Parent = Super::IsDataValid(Context);
	TMap<FName, const UWizardItemDefinition*> ById;
	bool bValid = Parent != EDataValidationResult::Invalid;
	for (const UWizardItemDefinition* Definition : Definitions)
	{
		if (!Definition || !Definition->IsConfigurationValid() || ById.Contains(Definition->ItemId))
		{
			Context.AddError(FText::FromString(TEXT("背包目录包含缺失、非法或重复ID的定义。")));
			bValid = false;
			continue;
		}
		ById.Add(Definition->ItemId, Definition);
	}
	TMap<FName, int64> Totals;
	for (const FWizardInitialItem& Item : InitialItems)
	{
		if (Item.Quantity <= 0 || !ById.Contains(Item.DefinitionId))
		{
			Context.AddError(FText::FromString(TEXT("初始库存包含未知定义或非正数数量。")));
			bValid = false;
			continue;
		}
		Totals.FindOrAdd(Item.DefinitionId) += Item.Quantity;
	}
	int64 SlotsRequired = 0;
	for (const auto& Pair : Totals)
	{
		const int64 Limit = ById[Pair.Key]->GetStackLimit();
		SlotsRequired += (Pair.Value + Limit - 1) / Limit;
	}
	if (SlotsRequired > UWizardInventoryComponent::Capacity)
	{
		Context.AddError(FText::FromString(TEXT("初始库存超过20格容量。")));
		bValid = false;
	}
	return bValid ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif

UWizardInventoryComponent::UWizardInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	State.Slots.SetNum(Capacity);
}

const UWizardItemDefinition* UWizardInventoryComponent::FindDefinition(FName Id) const
{
	const auto& Definitions = Config ? Config->Definitions : Catalog;
	for (const UWizardItemDefinition* Definition : Definitions)
	{
		if (Definition && Definition->ItemId == Id) return Definition;
	}
	return nullptr;
}

bool UWizardInventoryComponent::IsCatalogValid() const
{
	TSet<FName> Ids;
	const auto& Definitions = Config ? Config->Definitions : Catalog;
	for (const UWizardItemDefinition* Definition : Definitions)
	{
		if (!Definition || !Definition->IsConfigurationValid() || Ids.Contains(Definition->ItemId)) return false;
		Ids.Add(Definition->ItemId);
	}
	return true;
}

bool UWizardInventoryComponent::IsSnapshotValid(const FWizardInventorySnapshot& Snapshot) const
{
	if (Snapshot.Slots.Num() != Capacity || !IsCatalogValid()) return false;
	TSet<FGuid> Ids;
	for (const FWizardInventoryEntry& Entry : Snapshot.Slots)
	{
		if (Entry.IsEmpty()) continue;
		const UWizardItemDefinition* Definition = FindDefinition(Entry.DefinitionId);
		if (!Definition || !Entry.InstanceId.IsValid() || Entry.Quantity <= 0 || Entry.Quantity > Definition->GetStackLimit() || Ids.Contains(Entry.InstanceId)) return false;
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

EWizardInventoryResult UWizardInventoryComponent::InitializeInventory()
{
	if (bMutating) return EWizardInventoryResult::Busy;
	if (bInitialized) return EWizardInventoryResult::Success;
	if (!IsCatalogValid()) return EWizardInventoryResult::InvalidDefinition;
	// 在临时组件验证全量初值；失败不会向真实库存泄漏部分发放。
	UWizardInventoryComponent* Pending = NewObject<UWizardInventoryComponent>(this);
	Pending->Catalog = Config ? Config->Definitions : Catalog;
	const auto& Items = Config ? Config->InitialItems : InitialItems;
	for (const FWizardInitialItem& Item : Items)
	{
		int32 Accepted = 0;
		const EWizardInventoryResult Result = Pending->AddItems(Item.DefinitionId, Item.Quantity, Accepted);
		if (Result != EWizardInventoryResult::Success) return Result;
		if (Accepted != Item.Quantity) return EWizardInventoryResult::NoCapacity;
	}
	bInitialized = true;
	Commit(MoveTemp(Pending->State.Slots));
	return EWizardInventoryResult::Success;
}

EWizardInventoryResult UWizardInventoryComponent::AddItems(FName Id, int32 Quantity, int32& Accepted)
{
	Accepted = 0;
	if (bMutating) return EWizardInventoryResult::Busy;
	if (Quantity <= 0) return EWizardInventoryResult::InvalidQuantity;
	if (!IsCatalogValid()) return EWizardInventoryResult::InvalidDefinition;
	const UWizardItemDefinition* Definition = FindDefinition(Id);
	if (!Definition) return EWizardInventoryResult::UnknownDefinition;
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
	const UWizardItemDefinition* Definition = FindDefinition(Source.DefinitionId);
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
	if (!IsSnapshotValid(Snapshot)) return EWizardInventoryResult::InvalidSnapshot;
	bInitialized = true;
	TArray<FWizardInventoryEntry> Slots = Snapshot.Slots;
	Commit(MoveTemp(Slots));
	return EWizardInventoryResult::Success;
}

EWizardInventoryResult UWizardInventoryComponent::Use(FWizardInventoryRequest Request)
{
	const EWizardInventoryResult Result = ValidateRequest(Request);
	if (Result != EWizardInventoryResult::Success) return Result;
	const UWizardItemDefinition* Definition = FindDefinition(State.Slots[Request.Slot].DefinitionId);
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
	// 拆出地面堆叠需新身份；整堆交接保留原身份。
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
