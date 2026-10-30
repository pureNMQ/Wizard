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
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UFUNCTION(BlueprintPure, Category="Combat") UWizardAttributeSet* GetAttributes() const { return Attributes; }
	UFUNCTION(BlueprintPure, Category="Combat") bool IsDead() const;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat") TObjectPtr<class UWizardEnemyConfig> EnemyConfig;
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
