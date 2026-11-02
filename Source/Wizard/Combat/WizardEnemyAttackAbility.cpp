#include "Combat/WizardEnemyAttackAbility.h"
#include "Combat/WizardCombatPresentation.h"
#include "Combat/WizardEnemy.h"
#include "Combat/WizardEnemyProjectile.h"
#include "GAS/WizardDamageExecution.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "NativeGameplayTags.h"
#include "Engine/World.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_EnemyAttackCooldown, "Cooldown.Enemy.Attack");
UWizardEnemyDamageEffect::UWizardEnemyDamageEffect()
{
	DurationPolicy = EGameplayEffectDurationType::Instant;
	FGameplayEffectExecutionDefinition Execution;
	Execution.CalculationClass = UWizardDamageExecution::StaticClass();
	Executions.Add(Execution);
}
UWizardEnemyCooldownEffect::UWizardEnemyCooldownEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(1.f);
}
UWizardEnemyAttackAbility::UWizardEnemyAttackAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	CooldownGameplayEffectClass = UWizardEnemyCooldownEffect::StaticClass();
}
const FGameplayTagContainer* UWizardEnemyAttackAbility::GetCooldownTags() const
{
	static FGameplayTagContainer Tags(TAG_EnemyAttackCooldown);
	return &Tags;
}
void UWizardEnemyAttackAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	const AWizardEnemy* Enemy = Cast<AWizardEnemy>(ActorInfo->AvatarActor.Get());
	FGameplayEffectSpecHandle Spec = MakeOutgoingGameplayEffectSpec(UWizardEnemyCooldownEffect::StaticClass(), GetAbilityLevel());
	if (Spec.IsValid() && Enemy)
	{
		Spec.Data->SetDuration(FMath::Max(.1f, Enemy->AttackInterval), true);
		Spec.Data->DynamicGrantedTags.AddTag(TAG_EnemyAttackCooldown);
		ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, Spec);
	}
}
void UWizardEnemyAttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	bStruck = false;
	AWizardEnemy* Enemy = ActorInfo ? Cast<AWizardEnemy>(ActorInfo->AvatarActor.Get()) : nullptr;
	Target = Enemy ? Enemy->GetAttackTarget() : nullptr;
	if (!Enemy || !Enemy->CanAttackTarget(Target.Get()) || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	FGameplayCueParameters Windup; Windup.SourceObject = Enemy; Windup.Location = Enemy->GetActorLocation();
	ActorInfo->AbilitySystemComponent->AddGameplayCue(WizardCombat::WindupTag(), Windup);
	WindupTask = UAbilityTask_WaitDelay::WaitDelay(this, FMath::Max(.01f, Enemy->AttackWindup));
	WindupTask->OnFinish.AddDynamic(this, &ThisClass::OnStrike);
	WindupTask->ReadyForActivation();
	if (Enemy->AttackMontage)
	{
		MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Enemy->AttackMontage);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnInterrupted);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnInterrupted);
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnFinished);
		MontageTask->ReadyForActivation();
	}
}
void UWizardEnemyAttackAbility::OnStrike()
{
	if (!IsActive() || bStruck) { return; }
	bStruck = true;
	GetAbilitySystemComponentFromActorInfo()->RemoveGameplayCue(WizardCombat::WindupTag());
	AWizardEnemy* Enemy = Cast<AWizardEnemy>(GetAvatarActorFromActorInfo());
	if (Enemy && Enemy->CanAttackTarget(Target.Get()))
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(Enemy);
		Context.AddInstigator(Enemy, Enemy);
		WizardCombat::Burst(Enemy, Enemy->Presentation, EWizardCombatStage::Cast, Enemy->GetActorLocation() + Enemy->GetActorForwardVector() * 70.f);
		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(UWizardEnemyDamageEffect::StaticClass(), 1.f, Context);
		if (Enemy->bRanged)
		{
			FVector Direction = Target->GetActorLocation() - Enemy->GetActorLocation();
			Direction.Z = 0;
			const FTransform Transform(Direction.Rotation(), Enemy->GetActorLocation() + Direction.GetSafeNormal() * 70.f);
			AWizardEnemyProjectile* Projectile = Enemy->GetWorld()->SpawnActorDeferred<AWizardEnemyProjectile>(AWizardEnemyProjectile::StaticClass(), Transform, Enemy, Enemy, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (Projectile)
			{
				Projectile->Initialize(ASC, Spec, Enemy->ProjectileSpeed);
				Projectile->FinishSpawning(Transform);
			}
		}
		else if (IAbilitySystemInterface* TargetInterface = Cast<IAbilitySystemInterface>(Target.Get()))
		{
			ASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetInterface->GetAbilitySystemComponent());
			WizardCombat::Burst(Enemy, Enemy->Presentation, EWizardCombatStage::Impact, Target->GetActorLocation());
		}
	}
	if (!MontageTask) { OnFinished(); }
}
void UWizardEnemyAttackAbility::OnInterrupted() { EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true); }
void UWizardEnemyAttackAbility::OnFinished() { EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false); }
void UWizardEnemyAttackAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (!IsEndAbilityValid(Handle, ActorInfo)) { return; }
	if (ScopeLockCount > 0)
	{
		WaitingToExecute.Add(FPostLockDelegate::CreateUObject(this, &ThisClass::EndAbility, Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled));
		return;
	}
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid()) ActorInfo->AbilitySystemComponent->RemoveGameplayCue(WizardCombat::WindupTag());
	if (WindupTask) { WindupTask->OnFinish.RemoveDynamic(this, &ThisClass::OnStrike); }
	if (MontageTask)
	{
		MontageTask->OnInterrupted.RemoveDynamic(this, &ThisClass::OnInterrupted);
		MontageTask->OnCancelled.RemoveDynamic(this, &ThisClass::OnInterrupted);
		MontageTask->OnCompleted.RemoveDynamic(this, &ThisClass::OnFinished);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	WindupTask = nullptr;
	MontageTask = nullptr;
	Target.Reset();
}
