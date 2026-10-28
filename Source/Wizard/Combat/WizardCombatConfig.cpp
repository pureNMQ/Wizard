#include "Combat/WizardCombatConfig.h"
#include "GAS/WizardBasicAttackAbility.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

bool UWizardCombatConfig::IsConfigurationValid() const
{
	const UWizardBasicAttackAbility* Ability = BasicAttackAbilityClass ? Cast<UWizardBasicAttackAbility>(BasicAttackAbilityClass->GetDefaultObject()) : nullptr;
	return Ability && Ability->IsEffectConfigurationValid() && Ability->IsProjectileConfigurationValid()
		&& Ability->IsAnimationConfigurationValid()
		&& FMath::IsFinite(ProjectileSpeed) && ProjectileSpeed > 0.0f
		&& FMath::IsFinite(ProjectileLifetime) && ProjectileLifetime >= 0.1f
		&& !MuzzleOffset.ContainsNaN();
}

#if WITH_EDITOR
EDataValidationResult UWizardCombatConfig::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult ParentResult = Super::IsDataValid(Context);
	if (!IsConfigurationValid())
	{
		Context.AddError(FText::FromString(TEXT("战斗配置需要有效的普攻 Ability、可播放的攻击蒙太奇、可生成的弹丸类与伤害/冷却效果（静态冷却至少 0.05 秒）；弹速为正，存活至少 0.1 秒，出手偏移有效。")));
		return EDataValidationResult::Invalid;
	}
	return ParentResult == EDataValidationResult::Invalid ? ParentResult : EDataValidationResult::Valid;
}
#endif
