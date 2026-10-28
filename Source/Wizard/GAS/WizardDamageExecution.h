// Wizard 项目：伤害规则与 GAS 属性捕获、执行计算。
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "WizardDamageExecution.generated.h"

namespace WizardCombat
{
	WIZARD_API float CalculateDamage(float AttackPower, float Defense);
}

UCLASS()
class WIZARD_API UWizardDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
public:
	UWizardDamageExecution();
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& Parameters, FGameplayEffectCustomExecutionOutput& Output) const override;
};
