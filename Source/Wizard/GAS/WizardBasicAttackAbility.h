#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "WizardBasicAttackAbility.generated.h"

class AWizardProjectile;
class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;

UCLASS(Blueprintable)
class WIZARD_API UWizardBasicAttackAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UWizardBasicAttackAbility();
	// 蓝图配置效果类；行为、公式和每发执行状态仍由 C++ 管理。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat")
	TSubclassOf<AWizardProjectile> ProjectileClass;
	// 资源由能力蓝图指定；播放和出手事件的生命周期归本次 Ability。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat|Animation")
	TObjectPtr<UAnimMontage> AttackMontage;
	bool IsEffectConfigurationValid() const;
	bool IsProjectileConfigurationValid() const;
	bool IsAnimationConfigurationValid() const;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
private:
	UFUNCTION() void OnFireEvent(FGameplayEventData Payload);
	UFUNCTION() void OnMontageCompleted();
	UFUNCTION() void OnMontageInterrupted();
	bool SpawnProjectile();
	UPROPERTY(Transient) TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;
	UPROPERTY(Transient) TObjectPtr<UAbilityTask_WaitGameplayEvent> FireEventTask;
	bool bProjectileFired = false;
	bool bEndingAttack = false;
};
