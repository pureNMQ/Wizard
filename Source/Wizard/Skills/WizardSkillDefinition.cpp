#include "Skills/WizardSkillDefinition.h"
#include "Skills/WizardSkillAbility.h"
#include "Animation/AnimMontage.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

bool UWizardSkillDefinition::IsConfigurationValid() const
{
	const float Values[] = {ManaCost, Cooldown, Power, Range, Radius, Duration, SpeedMultiplier, ProjectileSpeed, EffectLifetime};
	for (float Value : Values) { if (!FMath::IsFinite(Value)) return false; }
	return !SkillId.IsNone() && AbilityClass && AbilityClass->IsChildOf(UWizardSkillAbility::StaticClass())
		&& !AbilityClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated) && CooldownTag.IsValid()
		&& Montage && Montage->GetPlayLength() > 0.f && Montage->RateScale > 0.f && ManaCost >= 0.f
		&& Cooldown >= .05f && Power >= 0.f && Range > 0.f && Radius > 0.f && Duration >= .05f
		&& SpeedMultiplier >= .1f && SpeedMultiplier <= 1.f && ProjectileSpeed > 0.f && EffectLifetime >= .05f
		&& !EffectScale.ContainsNaN() && EffectScale.GetMin() > 0.f
		&& (Behavior != EWizardSkillBehavior::Projectile || ProjectileFX);
}

#if WITH_EDITOR
EDataValidationResult UWizardSkillDefinition::IsDataValid(FDataValidationContext& Context) const
{
	if (!IsConfigurationValid())
	{
		Context.AddError(FText::FromString(TEXT("技能需要稳定ID、原生技能能力类、独立冷却Tag、可播放蒙太奇及有限有效数值；投射物需要粒子引用。")));
		return EDataValidationResult::Invalid;
	}
	return EDataValidationResult::Valid;
}
#endif

const UWizardSkillDefinition* UWizardSkillCatalog::Find(FName Id) const
{
	for (const UWizardSkillDefinition* Skill : Skills) { if (Skill && Skill->SkillId == Id) return Skill; }
	return nullptr;
}

bool UWizardSkillCatalog::IsConfigurationValid() const
{
	TSet<FName> Ids;
	TSet<FGameplayTag> Tags;
	for (const UWizardSkillDefinition* Skill : Skills)
	{
		if (!Skill || !Skill->IsConfigurationValid() || Ids.Contains(Skill->SkillId) || Tags.Contains(Skill->CooldownTag)) return false;
		Ids.Add(Skill->SkillId);
		Tags.Add(Skill->CooldownTag);
	}
	if (Skills.Num() < 6 || InitialLoadout.Slots.Num() != 4) return false;
	TSet<FName> Equipped;
	for (FName Id : InitialLoadout.Slots)
	{
		if (Id.IsNone()) continue;
		if (!Find(Id) || Equipped.Contains(Id)) return false;
		Equipped.Add(Id);
	}
	return true;
}
