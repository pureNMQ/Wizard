#include "Equipment/WizardEquipmentComponent.h"
#include "Inventory/WizardInventoryComponent.h"
#include "WizardPlayerState.h"
#include "WizardCharacter.h"
#include "GAS/WizardAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectAggregator.h"
#include "GameFramework/CharacterMovementComponent.h"

UWizardEquipmentComponent::UWizardEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	EffectHandles.SetNum(static_cast<int32>(EWizardEquipmentSlot::Count));
	AppliedDefinitions.SetNum(static_cast<int32>(EWizardEquipmentSlot::Count));
}

void UWizardEquipmentComponent::InitializeAvatar(APawn* Avatar)
{
	AWizardPlayerState* Player = Cast<AWizardPlayerState>(GetOwner());
	if (!Player || !Player->HasAuthority()) return;
	if (!Inventory.IsValid())
	{
		Inventory = Player->GetInventory();
		InventoryHandle = Inventory->OnChanged.AddUObject(this, &ThisClass::RefreshEquipment);
	}
	AWizardCharacter* NewCharacter = Cast<AWizardCharacter>(Avatar);
	if (Character.Get() != NewCharacter) ClearAvatar();
	Character = NewCharacter;
	RefreshEquipment();
	if (Character.IsValid())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = Player->GetAttributes()->GetMoveSpeed();
	}
}

void UWizardEquipmentComponent::RefreshEquipment()
{
	AWizardPlayerState* Player = Cast<AWizardPlayerState>(GetOwner());
	if (!Player || !Player->HasAuthority() || !Inventory.IsValid() || bRefreshing) return;
	TGuardValue<bool> Guard(bRefreshing, true);
	UAbilitySystemComponent* ASC = Player->GetAbilitySystemComponent();
	const FWizardInventorySnapshot Snapshot = Inventory->GetSnapshot();
	{
		// 同次换装只让聚合器看到最终上限，防止替换时先卸旧件造成额外生命损失。
		FScopedAggregatorOnDirtyBatch Batch;
		for (int32 Slot = 0; Slot < EffectHandles.Num(); ++Slot)
		{
			const UWizardItemDefinition* Definition = Snapshot.Equipment.IsValidIndex(Slot) ? Snapshot.Equipment[Slot].Definition.Get() : nullptr;
			const bool bEffectPresent = !EffectHandles[Slot].IsValid() || ASC->GetActiveGameplayEffect(EffectHandles[Slot]);
			if (AppliedDefinitions[Slot] == Definition && bEffectPresent) continue;
			FActiveGameplayEffectHandle NewHandle;
			if (Definition && !Definition->EquipmentBonuses.IsEmpty())
			{
				// 配置只读；每次应用的常量 modifier 属于独立运行时 GE 实例。
				UGameplayEffect* Effect = NewObject<UGameplayEffect>(this);
				Effect->DurationPolicy = EGameplayEffectDurationType::Infinite;
				for (const FWizardEquipmentBonus& Bonus : Definition->EquipmentBonuses)
				{
					FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
					Modifier.Attribute = Bonus.Attribute;
					Modifier.ModifierOp = EGameplayModOp::Additive;
					Modifier.ModifierMagnitude = FScalableFloat(Bonus.Magnitude);
				}
				FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
				Context.AddSourceObject(Definition);
				const FGameplayEffectSpec Spec(Effect, Context, 1.f);
				NewHandle = ASC->ApplyGameplayEffectSpecToSelf(Spec);
				if (!NewHandle.IsValid())
				{
					UE_LOG(LogTemp, Error, TEXT("装备效果应用失败：%s，槽位 %d"), *GetNameSafe(Definition), Slot);
					continue;
				}
			}
			if (EffectHandles[Slot].IsValid()) ASC->RemoveActiveGameplayEffect(EffectHandles[Slot]);
			EffectHandles[Slot] = NewHandle;
			AppliedDefinitions[Slot] = Definition;
		}
	}
	RefreshPresentation();
}

void UWizardEquipmentComponent::RefreshPresentation()
{
	if (!Character.IsValid() || !Inventory.IsValid()) return;
	const FWizardInventorySnapshot Snapshot = Inventory->GetSnapshot();
	const int32 StaffSlot = static_cast<int32>(EWizardEquipmentSlot::Staff);
	const UWizardItemDefinition* Staff = Snapshot.Equipment.IsValidIndex(StaffSlot) ? Snapshot.Equipment[StaffSlot].Definition.Get() : nullptr;
	Character->SetEquipmentStaff(Staff ? Staff->StaffMesh.LoadSynchronous() : nullptr);
}

void UWizardEquipmentComponent::ClearAvatar(AWizardCharacter* ExpectedAvatar)
{
	// 旧角色延迟销毁时不能清理已绑定的新角色。
	if (ExpectedAvatar && Character.Get() != ExpectedAvatar) return;
	if (Character.IsValid()) Character->SetEquipmentStaff(nullptr);
	Character.Reset();
}

void UWizardEquipmentComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Inventory.IsValid()) Inventory->OnChanged.Remove(InventoryHandle);
	InventoryHandle.Reset();
	Inventory.Reset();
	ClearAvatar();
	if (const AWizardPlayerState* Player = Cast<AWizardPlayerState>(GetOwner()))
	{
		for (const FActiveGameplayEffectHandle Handle : EffectHandles)
		{
			if (Handle.IsValid()) Player->GetAbilitySystemComponent()->RemoveActiveGameplayEffect(Handle);
		}
	}
	EffectHandles.Empty();
	AppliedDefinitions.Empty();
	Super::EndPlay(Reason);
}
