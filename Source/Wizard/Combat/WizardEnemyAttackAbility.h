// Wizard 项目：敌人攻击采用 GAS 提交、冷却和可取消的出手等待。
#pragma once
#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "WizardEnemyAttackAbility.generated.h"
UCLASS()
class WIZARD_API UWizardEnemyDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UWizardEnemyDamageEffect();
};
UCLASS()
class WIZARD_API UWizardEnemyCooldownEffect : public UGameplayEffect
{
	GENERATED_BODY()
public:
	UWizardEnemyCooldownEffect();
};
UCLASS()
class WIZARD_API UWizardEnemyAttackAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UWizardEnemyAttackAbility();
	virtual const FGameplayTagContainer* GetCooldownTags() const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
private:
	UFUNCTION() void OnStrike();
	UFUNCTION() void OnInterrupted();
	UFUNCTION() void OnFinished();
	UPROPERTY() TObjectPtr<class UAbilityTask_WaitDelay> WindupTask;
	UPROPERTY() TObjectPtr<class UAbilityTask_PlayMontageAndWait> MontageTask;
	TWeakObjectPtr<AActor> Target;
	bool bStruck = false;
};
