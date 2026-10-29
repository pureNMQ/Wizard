// Wizard 项目：运行时数值通过Spec注入，CDO保持只读。
#pragma once
#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "WizardSkillEffects.generated.h"
class UWizardSkillDefinition;
class UAbilitySystemComponent;
namespace WizardSkills
{
	// 范围内每个存活敌人结算一次；只由场景遮挡决定能否命中，敌人不互相挡伤害。
	WIZARD_API void ApplyAreaDamage(UAbilitySystemComponent* Source, const UWizardSkillDefinition* Definition, const FVector& Center, FGameplayEffectContextHandle Context);
	WIZARD_API FGameplayTag ValueTag();
	WIZARD_API FGameplayTag DamageScaleTag();
	WIZARD_API FGameplayTag CastingTag();
	WIZARD_API FGameplayTag FireTag();
}
UCLASS()
class WIZARD_API UWizardSkillTimedEffect : public UGameplayEffect
{
	GENERATED_BODY()
public: UWizardSkillTimedEffect();
};
UCLASS()
class WIZARD_API UWizardSkillManaEffect : public UGameplayEffect
{
	GENERATED_BODY()
public: UWizardSkillManaEffect();
};
UCLASS()
class WIZARD_API UWizardSkillShieldDurationEffect : public UWizardSkillTimedEffect
{
	GENERATED_BODY()
public: UWizardSkillShieldDurationEffect();
};
UCLASS()
class WIZARD_API UWizardSkillHealEffect : public UGameplayEffect
{
	GENERATED_BODY()
public: UWizardSkillHealEffect();
};
UCLASS()
class WIZARD_API UWizardSkillShieldEffect : public UGameplayEffect
{
	GENERATED_BODY()
public: UWizardSkillShieldEffect();
};
UCLASS()
class WIZARD_API UWizardSkillSlowEffect : public UGameplayEffect
{
	GENERATED_BODY()
public: UWizardSkillSlowEffect();
};
UCLASS()
class WIZARD_API UWizardSkillDamageEffect : public UGameplayEffect
{
	GENERATED_BODY()
public: UWizardSkillDamageEffect();
};
