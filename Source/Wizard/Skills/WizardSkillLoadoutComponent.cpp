#include "Skills/WizardSkillLoadoutComponent.h"
#include "Skills/WizardSkillEffects.h"
#include "Skills/WizardAlchemyStation.h"
#include "Interaction/WizardInteractionComponent.h"
#include "WizardPlayerState.h"
#include "WizardPlayerController.h"
#include "AbilitySystemComponent.h"
#include "GAS/WizardAttributeSet.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "Combat/WizardEnemy.h"

UWizardSkillLoadoutComponent::UWizardSkillLoadoutComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	static ConstructorHelpers::FObjectFinder<UWizardSkillCatalog> DefaultCatalog(TEXT("/Game/Wizard/Data/Skills/DA_SkillCatalog.DA_SkillCatalog"));
	Catalog = DefaultCatalog.Object;
}
AWizardPlayerState* UWizardSkillLoadoutComponent::GetState() const { return Cast<AWizardPlayerState>(GetOwner()); }
const UWizardSkillDefinition* UWizardSkillLoadoutComponent::Find(FName Id) const { return Catalog ? Catalog->Find(Id) : nullptr; }
const UWizardSkillDefinition* UWizardSkillLoadoutComponent::GetSlot(int32 Slot) const { return Loadout.Slots.IsValidIndex(Slot) ? Find(Loadout.Slots[Slot]) : nullptr; }

void UWizardSkillLoadoutComponent::InitializeAvatar()
{
	AWizardPlayerState* State = GetState();
	if (!State || !State->HasAuthority() || !Catalog || !Catalog->IsConfigurationValid()) return;
	UAbilitySystemComponent* ASC = State->GetAbilitySystemComponent();
	if (!EffectRemovedHandle.IsValid()) EffectRemovedHandle = ASC->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &ThisClass::OnEffectRemoved);
	if (!ShieldChangedHandle.IsValid()) ShieldChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetShieldAttribute()).AddUObject(this, &ThisClass::OnShieldChanged);
	for (UWizardSkillDefinition* Skill : Catalog->Skills)
	{
		if (!Handles.Contains(Skill->SkillId)) Handles.Add(Skill->SkillId, ASC->GiveAbility(FGameplayAbilitySpec(Skill->AbilityClass, 1, INDEX_NONE, Skill)));
	}
	if (Loadout.Slots.IsEmpty()) Loadout = Catalog->InitialLoadout;
	if (ACharacter* Avatar = Cast<ACharacter>(ASC->GetAvatarActor())) Avatar->GetCharacterMovement()->MaxWalkSpeed = State->GetAttributes()->GetMoveSpeed();
	OnLoadoutChanged.Broadcast();
}

bool UWizardSkillLoadoutComponent::CanEditLoadout() const
{
	return GetState() && !GetState()->IsInChallenge() && GetState()->GetInteraction()->FindAvailableTarget(AWizardAlchemyStation::StaticClass()) != nullptr;
}

EWizardSkillResult UWizardSkillLoadoutComponent::RestoreSnapshot(const FWizardLoadoutSnapshot& Snapshot)
{
	if (!GetState() || !GetState()->HasAuthority() || !Catalog) return EWizardSkillResult::InvalidConfiguration;
	if (Snapshot.Slots.Num() != 4) return EWizardSkillResult::InvalidSlot;
	TSet<FName> Ids;
	for (FName Id : Snapshot.Slots)
	{
		if (Id.IsNone()) continue;
		if (!Find(Id)) return EWizardSkillResult::UnknownSkill;
		if (Ids.Contains(Id)) return EWizardSkillResult::DuplicateSkill;
		Ids.Add(Id);
	}
	if (Snapshot.Slots != Loadout.Slots) { Loadout = Snapshot; OnLoadoutChanged.Broadcast(); }
	return EWizardSkillResult::Success;
}

EWizardSkillResult UWizardSkillLoadoutComponent::EquipSkill(FName Id, int32 Slot)
{
	if (!CanEditLoadout()) return EWizardSkillResult::NotAtAlchemy;
	if (!Loadout.Slots.IsValidIndex(Slot)) return EWizardSkillResult::InvalidSlot;
	if (!Find(Id)) return EWizardSkillResult::UnknownSkill;
	if (Loadout.Slots[Slot] == Id) return EWizardSkillResult::Success;
	if (Loadout.Slots.Contains(Id)) return EWizardSkillResult::DuplicateSkill;
	FWizardLoadoutSnapshot Updated = Loadout;
	Updated.Slots[Slot] = Id;
	return RestoreSnapshot(Updated);
}
EWizardSkillResult UWizardSkillLoadoutComponent::UnequipSkill(int32 Slot)
{
	if (!CanEditLoadout()) return EWizardSkillResult::NotAtAlchemy;
	if (!Loadout.Slots.IsValidIndex(Slot)) return EWizardSkillResult::InvalidSlot;
	FWizardLoadoutSnapshot Updated = Loadout;
	Updated.Slots[Slot] = NAME_None;
	return RestoreSnapshot(Updated);
}
EWizardSkillResult UWizardSkillLoadoutComponent::SwapSlots(int32 Source, int32 Target)
{
	if (!CanEditLoadout()) return EWizardSkillResult::NotAtAlchemy;
	if (!Loadout.Slots.IsValidIndex(Source) || !Loadout.Slots.IsValidIndex(Target)) return EWizardSkillResult::InvalidSlot;
	FWizardLoadoutSnapshot Updated = Loadout;
	Updated.Slots.Swap(Source, Target);
	return RestoreSnapshot(Updated);
}

float UWizardSkillLoadoutComponent::GetCooldownRemaining(const UWizardSkillDefinition* Definition) const
{
	if (!Definition || !GetState()) return 0.f;
	FGameplayTagContainer Tags(Definition->CooldownTag);
	const TArray<float> Times = GetState()->GetAbilitySystemComponent()->GetActiveEffectsTimeRemaining(FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(Tags));
	float Remaining = 0.f;
	for (float Time : Times) Remaining = FMath::Max(Remaining, Time);
	return Remaining;
}

EWizardSkillResult UWizardSkillLoadoutComponent::CheckSkill(const UWizardSkillDefinition* Definition, const FVector& Aim, FVector& OutTarget, bool bCheckAvailability) const
{
	const AWizardPlayerState* State = GetState();
	const ACharacter* Avatar = State ? Cast<ACharacter>(State->GetAbilitySystemComponent()->GetAvatarActor()) : nullptr;
	if (!Definition || !Definition->IsConfigurationValid() || !Avatar || !State->HasAuthority()) return EWizardSkillResult::InvalidConfiguration;
	const UWizardAttributeSet* Attributes = State->GetAttributes();
	if (Attributes->GetHealth() <= 0.f) return EWizardSkillResult::Dead;
	if (GetWorld()->IsPaused()) return EWizardSkillResult::Busy;
	if (bCheckAvailability && State->GetAbilitySystemComponent()->HasMatchingGameplayTag(WizardSkills::CastingTag())) return EWizardSkillResult::Busy;
	if (bCheckAvailability && GetCooldownRemaining(Definition) > 0.f) return EWizardSkillResult::CoolingDown;
	if (bCheckAvailability && Attributes->GetMana() < Definition->ManaCost) return EWizardSkillResult::NotEnoughMana;
	if (Definition->Behavior == EWizardSkillBehavior::Heal && Attributes->GetHealth() >= Attributes->GetMaxHealth()) return EWizardSkillResult::FullHealth;
	if (Aim.ContainsNaN()) return EWizardSkillResult::InvalidTarget;
	OutTarget = Aim;
	const FVector Origin = Avatar->GetActorLocation();
	FVector Direction = Aim - Origin;
	Direction.Z = 0.f;
	if (Definition->Behavior == EWizardSkillBehavior::Projectile)
	{
		return Direction.IsNearlyZero() ? EWizardSkillResult::InvalidTarget : EWizardSkillResult::Success;
	}
	if (Definition->Behavior != EWizardSkillBehavior::Area && Definition->Behavior != EWizardSkillBehavior::Blink) return EWizardSkillResult::Success;
	if (Direction.IsNearlyZero()) return EWizardSkillResult::InvalidTarget;
	if (Definition->Behavior == EWizardSkillBehavior::Area && Direction.Size() > Definition->Range) return EWizardSkillResult::InvalidTarget;
	if (Definition->Behavior == EWizardSkillBehavior::Blink) OutTarget = Origin + Direction.GetClampedToMaxSize(Definition->Range);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(SkillTarget), false, Avatar);
	FHitResult Ground;
	FCollisionObjectQueryParams FloorObjects;
	FloorObjects.AddObjectTypesToQuery(ECC_WorldStatic);
	FloorObjects.AddObjectTypesToQuery(ECC_WorldDynamic);
	if (!GetWorld()->LineTraceSingleByObjectType(Ground, OutTarget + FVector(0,0,1000), OutTarget - FVector(0,0,2000), FloorObjects, Query)
		|| !Avatar->GetCharacterMovement()->IsWalkable(Ground)) return EWizardSkillResult::InvalidTarget;
	OutTarget = Ground.ImpactPoint;
	FHitResult Block;
	if (Definition->Behavior == EWizardSkillBehavior::Blink)
	{
		OutTarget.Z += Avatar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 2.f;
		const FCollisionShape Capsule = FCollisionShape::MakeCapsule(Avatar->GetCapsuleComponent()->GetScaledCapsuleRadius(), Avatar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		if (GetWorld()->SweepSingleByChannel(Block, Origin, OutTarget, FQuat::Identity, ECC_Pawn, Capsule, Query)
			|| GetWorld()->OverlapBlockingTestByChannel(OutTarget, FQuat::Identity, ECC_Pawn, Capsule, Query)) return EWizardSkillResult::InvalidTarget;
	}
	else
	{
		FCollisionQueryParams Sight = Query;
		for (TActorIterator<AWizardEnemy> It(GetWorld()); It; ++It) Sight.AddIgnoredActor(*It);
		if (GetWorld()->LineTraceSingleByChannel(Block, Origin, OutTarget + FVector(0,0,20), ECC_Visibility, Sight)) return EWizardSkillResult::InvalidTarget;
	}
	return EWizardSkillResult::Success;
}

EWizardSkillResult UWizardSkillLoadoutComponent::ActivateSlot(int32 Slot)
{
	const UWizardSkillDefinition* Definition = GetSlot(Slot);
	if (!Definition) return EWizardSkillResult::InvalidSlot;
	AWizardPlayerState* State = GetState();
	FVector Target;
	const EWizardSkillResult Result = CheckSkill(Definition, State->GetAimPoint(), Target);
	if (Result != EWizardSkillResult::Success) return Result;
	const FGameplayAbilitySpecHandle* Handle = Handles.Find(Definition->SkillId);
	if (!Handle) return EWizardSkillResult::ActivationFailed;
	State->StopBasicAttack();
	return State->GetAbilitySystemComponent()->TryActivateAbility(*Handle) ? EWizardSkillResult::Success : EWizardSkillResult::ActivationFailed;
}

void UWizardSkillLoadoutComponent::CancelCasting()
{
	if (AWizardPlayerState* State = GetState())
	{
		FGameplayTagContainer Tags(WizardSkills::CastingTag());
		State->GetAbilitySystemComponent()->CancelAbilities(&Tags);
	}
}

void UWizardSkillLoadoutComponent::ApplyShield(float Amount, float Duration, const UWizardSkillDefinition* Definition)
{
	ClearAvatarEffects();
	UAbilitySystemComponent* ASC = GetState()->GetAbilitySystemComponent();
	FGameplayEffectSpecHandle Pool = ASC->MakeOutgoingSpec(UWizardSkillShieldEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
	Pool.Data->SetSetByCallerMagnitude(WizardSkills::ValueTag(), Amount);
	ASC->ApplyGameplayEffectSpecToSelf(*Pool.Data.Get());
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(Definition);
	FGameplayEffectSpecHandle Timer = ASC->MakeOutgoingSpec(UWizardSkillShieldDurationEffect::StaticClass(), 1.f, Context);
	Timer.Data->SetDuration(Duration, true);
	ShieldHandle = ASC->ApplyGameplayEffectSpecToSelf(*Timer.Data.Get());
}
void UWizardSkillLoadoutComponent::ClearAvatarEffects()
{
	if (!GetState()) return;
	UAbilitySystemComponent* ASC = GetState()->GetAbilitySystemComponent();
	const FActiveGameplayEffectHandle Previous = ShieldHandle;
	ShieldHandle.Invalidate();
	if (Previous.IsValid()) ASC->RemoveActiveGameplayEffect(Previous);
	FGameplayEffectSpecHandle Pool = ASC->MakeOutgoingSpec(UWizardSkillShieldEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
	Pool.Data->SetSetByCallerMagnitude(WizardSkills::ValueTag(), 0.f);
	ASC->ApplyGameplayEffectSpecToSelf(*Pool.Data.Get());
}
void UWizardSkillLoadoutComponent::OnEffectRemoved(const FActiveGameplayEffect& Effect)
{
	if (Effect.Handle == ShieldHandle) ClearAvatarEffects();
}
void UWizardSkillLoadoutComponent::OnShieldChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue <= 0.f && ShieldHandle.IsValid()) ClearAvatarEffects();
}
void UWizardSkillLoadoutComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	CancelCasting();
	ClearAvatarEffects();
	if (GetState())
	{
		UAbilitySystemComponent* ASC = GetState()->GetAbilitySystemComponent();
		ASC->OnAnyGameplayEffectRemovedDelegate().Remove(EffectRemovedHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetShieldAttribute()).Remove(ShieldChangedHandle);
	}
	OnLoadoutChanged.Clear();
	Super::EndPlay(Reason);
}
