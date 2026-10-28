#include "GAS/WizardDamageExecution.h"
#include "GAS/WizardAttributeSet.h"
#include "GameplayEffect.h"

namespace
{
	struct FDamageCaptures
	{
		DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower);
		DECLARE_ATTRIBUTE_CAPTUREDEF(Defense);
		FDamageCaptures()
		{
			DEFINE_ATTRIBUTE_CAPTUREDEF(UWizardAttributeSet, AttackPower, Source, true);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UWizardAttributeSet, Defense, Target, false);
		}
	};
	const FDamageCaptures& Captures()
	{
		static FDamageCaptures Instance;
		return Instance;
	}
}

float WizardCombat::CalculateDamage(float AttackPower, float Defense)
{
	if (!FMath::IsFinite(AttackPower) || !FMath::IsFinite(Defense) || AttackPower <= 0.0f)
	{
		return 0.0f;
	}
	return FMath::Max(1.0f, AttackPower - FMath::Max(0.0f, Defense));
}

UWizardDamageExecution::UWizardDamageExecution()
{
	RelevantAttributesToCapture.Add(Captures().AttackPowerDef);
	RelevantAttributesToCapture.Add(Captures().DefenseDef);
}

void UWizardDamageExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& Parameters, FGameplayEffectCustomExecutionOutput& Output) const
{
	FAggregatorEvaluateParameters Evaluation;
	Evaluation.SourceTags = Parameters.GetOwningSpec().CapturedSourceTags.GetAggregatedTags();
	Evaluation.TargetTags = Parameters.GetOwningSpec().CapturedTargetTags.GetAggregatedTags();
	float Attack = 0.0f;
	float Defense = 0.0f;
	Parameters.AttemptCalculateCapturedAttributeMagnitude(Captures().AttackPowerDef, Evaluation, Attack);
	Parameters.AttemptCalculateCapturedAttributeMagnitude(Captures().DefenseDef, Evaluation, Defense);
	Output.AddOutputModifier(FGameplayModifierEvaluatedData(UWizardAttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, WizardCombat::CalculateDamage(Attack, Defense)));
}
