// Wizard 项目：玩家持续战斗状态属于 PlayerState，Pawn 仅充当 Avatar。
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpec.h"
#include "Engine/TimerHandle.h"
#include "WizardPlayerState.generated.h"

class UWizardAttributeSet;
class UWizardInventoryComponent;
class UWizardCombatConfig;
class UWizardSkillLoadoutComponent;
class UWizardInteractionComponent;
struct FOnAttributeChangeData;
struct FAbilityEndedData;

UCLASS()
class WIZARD_API AWizardPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	AWizardPlayerState();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UFUNCTION(BlueprintPure, Category="Combat") UWizardAttributeSet* GetAttributes() const { return Attributes; }
	// 同一 Avatar 重复初始化无副作用；替换 Avatar 前取消旧输入和能力。
	void InitializeAvatar(APawn* Avatar);
	UFUNCTION(BlueprintCallable, Category="Combat") void StartBasicAttack();
	// 松开只停止后续连射，已开始的一发继续到动画结束。
	UFUNCTION(BlueprintCallable, Category="Combat") void ReleaseBasicAttack();
	// 界面、死亡、输入取消或 Avatar 失效时，连同未出手的当前一发强制取消。
	UFUNCTION(BlueprintCallable, Category="Combat") void StopBasicAttack();
	void SetAimPoint(const FVector& Point) { AimPoint = Point; }
	FVector GetAimPoint() const { return AimPoint; }
	const UWizardCombatConfig* GetCombatConfig() const;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat") TObjectPtr<UWizardCombatConfig> CombatConfig;
	UFUNCTION(BlueprintPure, Category="Skills") UWizardSkillLoadoutComponent* GetSkills() const { return Skills; }
	UFUNCTION(BlueprintPure, Category="Interaction") UWizardInteractionComponent* GetInteraction() const { return Interaction; }
	UFUNCTION(BlueprintPure, Category="Challenge") bool IsInChallenge() const { return bInChallenge; }
	void SetInChallenge(bool bActive) { bInChallenge = bActive; }
	// 挑战完成/死亡共用恢复入口；不改搭配与冷却，不重授予能力。
	void RestoreAtCamp();
	UFUNCTION(BlueprintPure, Category="Inventory") UWizardInventoryComponent* GetInventory() const { return Inventory; }
protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	void TryBasicAttack();
	void QueueBasicAttack();
	void OnBasicAttackEnded(const FAbilityEndedData& Data);
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnCooldownChanged(const FGameplayTag Tag, int32 Count);
	UPROPERTY(VisibleAnywhere, Category="Combat") TObjectPtr<UAbilitySystemComponent> AbilitySystem;
	UPROPERTY() TObjectPtr<UWizardAttributeSet> Attributes;
	UPROPERTY(VisibleAnywhere, Category="Skills") TObjectPtr<UWizardSkillLoadoutComponent> Skills;
	UPROPERTY(VisibleAnywhere, Category="Interaction") TObjectPtr<UWizardInteractionComponent> Interaction;
	UPROPERTY(VisibleAnywhere, Category="Inventory") TObjectPtr<UWizardInventoryComponent> Inventory;
	FGameplayAbilitySpecHandle BasicAttackHandle;
	FDelegateHandle HealthDelegate;
	FDelegateHandle CooldownDelegate;
	FDelegateHandle AbilityEndedDelegate;
	FTimerHandle RetryAttackTimer;
	bool bAttackHeld = false;
	bool bInChallenge = false;
	FVector AimPoint = FVector::ZeroVector;
};
