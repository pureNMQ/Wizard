// Wizard 项目：T02 静止受击目标；AI 行为由 T09 扩展。
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "WizardEnemy.generated.h"

class UWizardAttributeSet;
struct FOnAttributeChangeData;

UCLASS(Blueprintable)
class WIZARD_API AWizardEnemy : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	AWizardEnemy();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UFUNCTION(BlueprintPure, Category="Combat") UWizardAttributeSet* GetAttributes() const { return Attributes; }
	UFUNCTION(BlueprintPure, Category="Combat") bool IsDead() const;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	UFUNCTION(BlueprintImplementableEvent, Category="Combat") void OnDeath();
private:
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	UPROPERTY(VisibleAnywhere, Category="Combat") TObjectPtr<UAbilitySystemComponent> AbilitySystem;
	UPROPERTY() TObjectPtr<UWizardAttributeSet> Attributes;
	FDelegateHandle HealthDelegate;
};
