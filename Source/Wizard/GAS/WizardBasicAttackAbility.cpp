#include "GAS/WizardBasicAttackAbility.h"
#include "AbilitySystemComponent.h"
#include "Combat/WizardCombatConfig.h"
#include "Combat/WizardProjectile.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "GAS/WizardAttributeSet.h"
#include "GAS/WizardCombatTags.h"
#include "GAS/WizardDamageExecution.h"
#include "WizardPlayerState.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

DEFINE_LOG_CATEGORY_STATIC(LogWizardBasicAttack, Log, All);

UWizardBasicAttackAbility::UWizardBasicAttackAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

bool UWizardBasicAttackAbility::IsEffectConfigurationValid() const
{
	const UGameplayEffect* Damage = DamageEffectClass ? DamageEffectClass->GetDefaultObject<UGameplayEffect>() : nullptr;
	const UGameplayEffect* Cooldown = GetCooldownGameplayEffect();
	if (!Damage || Damage->DurationPolicy != EGameplayEffectDurationType::Instant
		|| !Cooldown || Cooldown->DurationPolicy != EGameplayEffectDurationType::HasDuration
		|| !Cooldown->GetGrantedTags().HasTagExact(WizardCombat::BasicAttackCooldownTag())
		|| GetCostGameplayEffect() != nullptr)
	{
		return false;
	}
	// 当前普攻采用静态、有限正持续时间；拒绝零冷却，避免标签移除事件递归续发。
	float Duration = 0.0f;
	if (!Cooldown->DurationMagnitude.GetStaticMagnitudeIfPossible(GetAbilityLevel(), Duration)
		|| !FMath::IsFinite(Duration) || Duration < 0.05f)
	{
		return false;
	}
	return Damage->Executions.ContainsByPredicate([](const FGameplayEffectExecutionDefinition& Execution)
	{
		return Execution.CalculationClass && Execution.CalculationClass->IsChildOf(UWizardDamageExecution::StaticClass());
	});
}

bool UWizardBasicAttackAbility::IsProjectileConfigurationValid() const
{
	return ProjectileClass && !ProjectileClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists);
}

bool UWizardBasicAttackAbility::IsAnimationConfigurationValid() const
{
	return AttackMontage && FMath::IsFinite(AttackMontage->GetPlayLength()) && AttackMontage->GetPlayLength() > 0.0f
		&& FMath::IsFinite(AttackMontage->RateScale) && AttackMontage->RateScale > 0.0f;
}

void UWizardBasicAttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	bEndingAttack = false;
	bProjectileFired = false;
	AWizardPlayerState* State = ActorInfo ? Cast<AWizardPlayerState>(ActorInfo->OwnerActor.Get()) : nullptr;
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!State || !IsValid(Avatar) || !IsEffectConfigurationValid() || !IsProjectileConfigurationValid()
		|| !IsAnimationConfigurationValid() || State->GetAttributes()->GetHealth() <= 0.0f)
	{
		UE_LOG(LogWizardBasicAttack, Warning, TEXT("普通攻击启动失败：角色或效果、弹丸、蒙太奇配置无效。"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	// 先监听再播放，保证首帧通知也能被本次 Ability 接收。
	FireEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, WizardCombat::BasicAttackFireTag(), nullptr, false, true);
	FireEventTask->EventReceived.AddDynamic(this, &ThisClass::OnFireEvent);
	FireEventTask->ReadyForActivation();
	MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AttackMontage, 1.0f,
		NAME_None, true, 1.0f, 0.0f, true);
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnMontageCompleted);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnMontageInterrupted);
	MontageTask->ReadyForActivation();
	// 不在此处结束：出手通知和动画完成/中断回调共同推进本次执行
}

void UWizardBasicAttackAbility::OnFireEvent(FGameplayEventData Payload)
{
	if (!IsActive() || bEndingAttack || bProjectileFired) { return; }
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	AWizardPlayerState* State = ActorInfo ? Cast<AWizardPlayerState>(ActorInfo->OwnerActor.Get()) : nullptr;
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	// 事件必须来自本次 Avatar 正在播放的攻击蒙太奇；别的动作不能借同名 Tag 出手。
	if (!State || !IsValid(Avatar) || !ASC || State->GetAttributes()->GetHealth() <= 0.0f
		|| ASC->GetAvatarActor() != Avatar || Payload.Instigator != Avatar || Payload.OptionalObject != AttackMontage
		|| ASC->GetAnimatingAbility() != this || ASC->GetCurrentMontage() != AttackMontage)
	{
		return;
	}
	// 生成 Actor 会同步触发用户回调；先消费出手事件，防重入/重复 Notify 生成第二发。
	bProjectileFired = true;
	if (!SpawnProjectile())
	{
		UE_LOG(LogWizardBasicAttack, Warning, TEXT("普通攻击出手失败：无法生成配置的弹丸或伤害 Spec。"));
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
	}
}

bool UWizardBasicAttackAbility::SpawnProjectile()
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	AWizardPlayerState* State = ActorInfo ? Cast<AWizardPlayerState>(ActorInfo->OwnerActor.Get()) : nullptr;
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!State || !IsValid(Avatar) || !ASC || !Avatar->HasAuthority()) { return false; }
	const UWizardCombatConfig* Config = State->GetCombatConfig();
	const FVector Origin = Avatar->GetActorLocation() + Avatar->GetActorRotation().RotateVector(Config->MuzzleOffset);
	FVector Direction = State->GetAimPoint() - Avatar->GetActorLocation();
	Direction.Z = 0.0f;
	Direction = Direction.GetSafeNormal();
	if (Direction.IsNearlyZero())
	{
		Direction = Avatar->GetActorForwardVector();
	}
	FTransform Transform(Direction.Rotation(), Origin);
	AWizardProjectile* Projectile = Avatar->GetWorld()->SpawnActorDeferred<AWizardProjectile>(ProjectileClass, Transform, Avatar, Cast<APawn>(Avatar), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (Projectile)
	{
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(this);
		Context.AddInstigator(Avatar, Projectile);
		const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, Context);
		if (!Spec.IsValid())
		{
			Projectile->Destroy();
			return false;
		}
		Projectile->InitializeProjectile(ASC, Spec, Config);
		Projectile->FinishSpawning(Transform);
	}
	return Projectile != nullptr;
}

void UWizardBasicAttackAbility::OnMontageCompleted()
{
	if (!IsActive() || bEndingAttack) { return; }
	if (!bProjectileFired)
	{
		// 缺失/配置错误的通知必须可诊断，并终止连射，不能静默循环空放。
		UE_LOG(LogWizardBasicAttack, Warning, TEXT("攻击蒙太奇 %s 播放完毕但未收到有效出手事件 %s，请检查通知。"),
			*GetNameSafe(AttackMontage), *WizardCombat::BasicAttackFireTag().ToString());
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, !bProjectileFired);
}

void UWizardBasicAttackAbility::OnMontageInterrupted()
{
	if (!IsActive() || bEndingAttack) { return; }
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UWizardBasicAttackAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (bEndingAttack || !IsEndAbilityValid(Handle, ActorInfo)) { return; }
	if (ScopeLockCount > 0)
	{
		// 遵守 GAS 的作用域锁约定；真正清理前不设结束标记，否则延后的调用会被自己拦下。
		WaitingToExecute.Add(FPostLockDelegate::CreateUObject(this, &ThisClass::EndAbility,
			Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled));
		return;
	}
	bEndingAttack = true;
	// Super 会结束所有 AbilityTask（含停止本 Ability 的蒙太奇）；先解除业务回调防重入。
	if (FireEventTask)
	{
		FireEventTask->EventReceived.RemoveDynamic(this, &ThisClass::OnFireEvent);
	}
	if (MontageTask)
	{
		MontageTask->OnCompleted.RemoveDynamic(this, &ThisClass::OnMontageCompleted);
		MontageTask->OnInterrupted.RemoveDynamic(this, &ThisClass::OnMontageInterrupted);
		MontageTask->OnCancelled.RemoveDynamic(this, &ThisClass::OnMontageInterrupted);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	FireEventTask = nullptr;
	MontageTask = nullptr;
}
