#include "Skills/WizardSkillAbility.h"
#include "Combat/WizardCombatPresentation.h"
#include "Skills/WizardSkillDefinition.h"
#include "Skills/WizardSkillEffects.h"
#include "Skills/WizardSkillLoadoutComponent.h"
#include "Skills/WizardSkillCue.h"
#include "WizardPlayerState.h"
#include "Combat/WizardProjectile.h"
#include "Combat/WizardCombatConfig.h"
#include "GAS/WizardAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

UWizardSkillAbility::UWizardSkillAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	AbilityTags.AddTag(WizardSkills::CastingTag());
	ActivationOwnedTags.AddTag(WizardSkills::CastingTag());
	ActivationBlockedTags.AddTag(WizardSkills::CastingTag());
}
const UWizardSkillDefinition* UWizardSkillAbility::GetDefinition(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	return Cast<UWizardSkillDefinition>(GetSourceObject(Handle, ActorInfo));
}
bool UWizardSkillAbility::CanActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags)) return false;
	const AWizardPlayerState* State = ActorInfo ? Cast<AWizardPlayerState>(ActorInfo->OwnerActor.Get()) : nullptr;
	FVector Target;
	return State && State->GetSkills()->CheckSkill(GetDefinition(Handle, ActorInfo), State->GetAimPoint(), Target) == EWizardSkillResult::Success;
}
bool UWizardSkillAbility::CheckCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UWizardSkillDefinition* Definition = GetDefinition(Handle, ActorInfo);
	return Definition && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid()
		&& ActorInfo->AbilitySystemComponent->GetNumericAttribute(UWizardAttributeSet::GetManaAttribute()) >= Definition->ManaCost;
}
bool UWizardSkillAbility::CheckCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	const UWizardSkillDefinition* Definition = GetDefinition(Handle, ActorInfo);
	return Definition && ActorInfo && ActorInfo->AbilitySystemComponent.IsValid()
		&& !ActorInfo->AbilitySystemComponent->HasMatchingGameplayTag(Definition->CooldownTag);
}
void UWizardSkillAbility::ApplyCost(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const
{
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UWizardSkillManaEffect::StaticClass(), 1.f, MakeEffectContext(Handle, ActorInfo));
	Spec.Data->SetSetByCallerMagnitude(WizardSkills::ValueTag(), -GetDefinition(Handle, ActorInfo)->ManaCost);
	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}
void UWizardSkillAbility::ApplyCooldown(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo) const
{
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	const UWizardSkillDefinition* Definition = GetDefinition(Handle, ActorInfo);
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UWizardSkillTimedEffect::StaticClass(), 1.f, MakeEffectContext(Handle, ActorInfo));
	Spec.Data->SetDuration(Definition->Cooldown, true);
	Spec.Data->DynamicGrantedTags.AddTag(Definition->CooldownTag);
	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}
void UWizardSkillAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	bEnding = false;
	bFired = false;
	AWizardPlayerState* State = Cast<AWizardPlayerState>(ActorInfo->OwnerActor.Get());
	const UWizardSkillDefinition* Definition = GetDefinition(Handle, ActorInfo);
	if (!State || !Definition || State->GetSkills()->CheckSkill(Definition, State->GetAimPoint(), TargetPoint, false) != EWizardSkillResult::Success
		|| !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	FGameplayCueParameters Windup;
 Windup.SourceObject = Definition; Windup.Location = Definition->Behavior == EWizardSkillBehavior::Area ? TargetPoint : ActorInfo->AvatarActor->GetActorLocation();
 Windup.RawMagnitude = Definition->Behavior == EWizardSkillBehavior::Area ? Definition->Radius : 0;
 ActorInfo->AbilitySystemComponent->AddGameplayCue(WizardCombat::WindupTag(), Windup);
 FireTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, WizardSkills::FireTag(), nullptr, false, true);
	FireTask->EventReceived.AddDynamic(this, &ThisClass::OnFire);
	FireTask->ReadyForActivation();
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Definition->Montage, 1.f, NAME_None, true, 1.f, 0.f, true);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnInterrupted);
	MontageTask->ReadyForActivation();
}
void UWizardSkillAbility::OnFire(FGameplayEventData Payload)
{
	const UWizardSkillDefinition* Definition = GetDefinition(CurrentSpecHandle, CurrentActorInfo);
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!IsActive() || bEnding || bFired || !Definition || !ASC || !Avatar || Payload.Instigator != Avatar
		|| Payload.OptionalObject != Definition->Montage || ASC->GetAnimatingAbility() != this || ASC->GetCurrentMontage() != Definition->Montage) return;
	bFired = true;
	ASC->RemoveGameplayCue(WizardCombat::WindupTag());
	if (!ExecuteSkill()) EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
bool UWizardSkillAbility::ExecuteSkill()
{
	const UWizardSkillDefinition* Definition = GetDefinition(CurrentSpecHandle, CurrentActorInfo);
	AWizardPlayerState* State = Cast<AWizardPlayerState>(GetOwningActorFromActorInfo());
	ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!Definition || !State || !Avatar || State->GetAttributes()->GetHealth() <= 0.f) return false;
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(Definition);
	Context.AddInstigator(Avatar, Avatar);
	auto Burst = [&](const FVector& Point)
	{
		FGameplayCueParameters Cue;
		Cue.SourceObject = Definition;
		Cue.Location = Point;
		Cue.Normal = FVector::UpVector;
		ASC->ExecuteGameplayCue(WizardSkills::ImpactCueTag(), Cue);
	};
	if (Definition->Behavior == EWizardSkillBehavior::Projectile)
	{
		FVector Direction = TargetPoint - Avatar->GetActorLocation();
		Direction.Z = 0.f;
		const FVector Origin = Avatar->GetActorLocation() + Avatar->GetActorRotation().RotateVector(State->GetCombatConfig()->MuzzleOffset);
		FTransform Transform(Direction.Rotation(), Origin);
		AWizardProjectile* Projectile = GetWorld()->SpawnActorDeferred<AWizardProjectile>(AWizardProjectile::StaticClass(), Transform, Avatar, Avatar, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Projectile) return false;
		Context.AddInstigator(Avatar, Projectile);
		FGameplayEffectSpecHandle Damage = ASC->MakeOutgoingSpec(UWizardSkillDamageEffect::StaticClass(), 1.f, Context);
		Damage.Data->SetSetByCallerMagnitude(WizardSkills::DamageScaleTag(), Definition->Power);
		Projectile->InitializeSkillProjectile(ASC, Damage, Definition);
		Projectile->FinishSpawning(Transform);
		WizardSkills::PlayCastSound(Avatar, Definition);
		return true;
	}
	if (Definition->Behavior == EWizardSkillBehavior::Area)
	{
		WizardSkills::ApplyAreaDamage(ASC, Definition, TargetPoint, Context);
		Burst(TargetPoint);
		WizardCombat::Burst(Avatar, Definition->Presentation, EWizardCombatStage::Arrival, TargetPoint);
	}
	else if (Definition->Behavior == EWizardSkillBehavior::Blink)
	{
		// 出手前障碍可能变化，重新检查整段胶囊通道；不依赖特效决定落点。
		FCollisionQueryParams Query(SCENE_QUERY_STAT(BlinkFire), false, Avatar);
		FHitResult Hit;
		const FCollisionShape Capsule = FCollisionShape::MakeCapsule(Avatar->GetCapsuleComponent()->GetScaledCapsuleRadius(), Avatar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
		if (GetWorld()->SweepSingleByChannel(Hit, Avatar->GetActorLocation(), TargetPoint, FQuat::Identity, ECC_Pawn, Capsule, Query)
			|| GetWorld()->OverlapBlockingTestByChannel(TargetPoint, FQuat::Identity, ECC_Pawn, Capsule, Query)) return false;
		const FVector From = Avatar->GetActorLocation();
		if (!Avatar->TeleportTo(TargetPoint, Avatar->GetActorRotation(), false, true)) return false;
		if (Definition->Presentation)
  {
   WizardCombat::Burst(Avatar, Definition->Presentation, EWizardCombatStage::Departure, From);
   WizardCombat::Burst(Avatar, Definition->Presentation, EWizardCombatStage::Arrival, TargetPoint);
  }
  else { Burst(From); Burst(TargetPoint); }
	}
	else if (Definition->Behavior == EWizardSkillBehavior::Heal)
	{
		FGameplayEffectSpecHandle Heal = ASC->MakeOutgoingSpec(UWizardSkillHealEffect::StaticClass(), 1.f, Context);
		Heal.Data->SetSetByCallerMagnitude(WizardSkills::ValueTag(), Definition->Power);
		ASC->ApplyGameplayEffectSpecToSelf(*Heal.Data.Get());
		Burst(Avatar->GetActorLocation() - FVector(0,0,Avatar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()));
	}
	else if (Definition->Behavior == EWizardSkillBehavior::Shield)
	{
		State->GetSkills()->ApplyShield(Definition->Power, Definition->Duration, Definition);
	}
	WizardSkills::PlayCastSound(Avatar, Definition);
	return true;
}
void UWizardSkillAbility::OnCompleted()
{
	if (!bFired) UE_LOG(LogTemp, Warning, TEXT("技能蒙太奇结束但未收到有效出手事件，请检查技能Notify。"));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, !bFired);
}
void UWizardSkillAbility::OnInterrupted() { EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); }
void UWizardSkillAbility::EndAbility(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bEnding || !IsEndAbilityValid(Handle, ActorInfo)) return;
	if (ScopeLockCount > 0)
	{
		WaitingToExecute.Add(FPostLockDelegate::CreateUObject(this, &ThisClass::EndAbility, Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled));
		return;
	}
	bEnding = true;
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid()) ActorInfo->AbilitySystemComponent->RemoveGameplayCue(WizardCombat::WindupTag());
	if (FireTask) FireTask->EventReceived.RemoveDynamic(this, &ThisClass::OnFire);
	if (MontageTask)
	{
		MontageTask->OnCompleted.RemoveDynamic(this, &ThisClass::OnCompleted);
		MontageTask->OnInterrupted.RemoveDynamic(this, &ThisClass::OnInterrupted);
		MontageTask->OnCancelled.RemoveDynamic(this, &ThisClass::OnInterrupted);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	FireTask = nullptr;
	MontageTask = nullptr;
}
