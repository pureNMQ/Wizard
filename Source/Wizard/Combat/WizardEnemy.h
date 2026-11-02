// Wizard 项目：怪物拥有独立 ASC；挑战怪物通过配置行为树运行。
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpecHandle.h"
#include "WizardEnemy.generated.h"

class UWizardAttributeSet;
struct FOnAttributeChangeData;
class AWizardEnemy;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWizardEnemyDefeated, AWizardEnemy*, Enemy);

UCLASS(Blueprintable)
class WIZARD_API AWizardEnemy : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	AWizardEnemy();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation") TObjectPtr<class UWizardCombatPresentation> Presentation;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UFUNCTION(BlueprintPure, Category="Combat") UWizardAttributeSet* GetAttributes() const { return Attributes; }
	UFUNCTION(BlueprintPure, Category="Combat") bool IsDead() const;
	// 蓝图只保存初始配置；生成后的当前属性以ASC为唯一来源。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Attributes", meta=(ClampMin="1")) float InitialHealth = 100.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Attributes", meta=(ClampMin="0")) float InitialAttackPower = 12.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Attributes", meta=(ClampMin="0")) float InitialDefense = 0.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Attributes", meta=(ClampMin="1")) float InitialMoveSpeed = 240.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|AI") TObjectPtr<class UBehaviorTree> BehaviorTree;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Attack", meta=(ClampMin="80")) float AttackRange = 160.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Attack", meta=(ClampMin="0.1")) float AttackInterval = 1.5f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Attack", meta=(ClampMin="0.01")) float AttackWindup = .35f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Attack") bool bRanged = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Attack", meta=(ClampMin="1", EditCondition="bRanged")) float ProjectileSpeed = 700.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Animation") TObjectPtr<class UAnimMontage> AttackMontage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Animation") TObjectPtr<class UAnimSequence> DeathAnimation;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Enemy|Loot") TObjectPtr<class UWizardDropConfig> DropConfig;
	// 挑战敌人必须有有效战斗数值及行为树；无行为树的独立靶子仍可受伤。
	bool IsConfigurationValid() const;
	// Pawn 的Owner会被占有流程改为Controller；挑战归属必须独立保存。
	UPROPERTY(Transient) TWeakObjectPtr<class AWizardChallengeArena> SourceChallenge;
	UPROPERTY(BlueprintAssignable, Category="Combat") FWizardEnemyDefeated OnEnemyDefeated;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Combat") TObjectPtr<class UWidgetComponent> EnemyHealthBar;
	// 仅接受存活玩家；行为树和能力共用距离及视线校验。
	bool CanAttackTarget(const AActor* Target) const;
	bool RequestAttack(AActor* Target);
	AActor* GetAttackTarget() const { return AttackTarget.Get(); }
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	UFUNCTION(BlueprintImplementableEvent, Category="Combat") void OnDeath();
private:
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	UPROPERTY(VisibleAnywhere, Category="Combat") TObjectPtr<UAbilitySystemComponent> AbilitySystem;
	UPROPERTY() TObjectPtr<UWizardAttributeSet> Attributes;
	FDelegateHandle HealthDelegate;
	FGameplayAbilitySpecHandle AttackHandle;
	TWeakObjectPtr<AActor> AttackTarget;
	bool bDeathHandled = false;
};
