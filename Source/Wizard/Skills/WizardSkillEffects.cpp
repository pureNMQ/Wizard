#include "Skills/WizardSkillEffects.h"
#include "Combat/WizardCombatPresentation.h"
#include "GAS/WizardAttributeSet.h"
#include "GAS/WizardDamageExecution.h"
#include "NativeGameplayTags.h"
#include "Skills/WizardSkillCue.h"
#include "Skills/WizardSkillDefinition.h"
#include "Combat/WizardEnemy.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "EngineUtils.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SkillValue, "Data.Wizard.Skill.Value");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SkillDamageScale, "Data.Wizard.Skill.DamageScale");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SkillCasting, "State.Wizard.Casting");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SkillFire, "Event.Wizard.Skill.Fire");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_FireballCooldown, "Cooldown.Wizard.Fireball");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_IceArrowCooldown, "Cooldown.Wizard.IceArrow");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_LightningCooldown, "Cooldown.Wizard.Lightning");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_BlinkCooldown, "Cooldown.Wizard.Blink");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_ShieldCooldown, "Cooldown.Wizard.Shield");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_HealCooldown, "Cooldown.Wizard.Heal");
FGameplayTag WizardSkills::ValueTag() { return TAG_SkillValue; }
FGameplayTag WizardSkills::DamageScaleTag() { return TAG_SkillDamageScale; }
FGameplayTag WizardSkills::CastingTag() { return TAG_SkillCasting; }
FGameplayTag WizardSkills::FireTag() { return TAG_SkillFire; }

namespace
{
	void AddValueModifier(UGameplayEffect* Effect, FGameplayAttribute Attribute, EGameplayModOp::Type Operation)
	{
		FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = Operation;
		FSetByCallerFloat Value;
		Value.DataTag = WizardSkills::ValueTag();
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(Value);
	}
}
UWizardSkillTimedEffect::UWizardSkillTimedEffect()
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(1.f);
}
UWizardSkillManaEffect::UWizardSkillManaEffect() { AddValueModifier(this, UWizardAttributeSet::GetManaAttribute(), EGameplayModOp::Additive); }
UWizardSkillShieldDurationEffect::UWizardSkillShieldDurationEffect()
{
	FGameplayEffectCue& Cue = GameplayCues.AddDefaulted_GetRef();
	Cue.GameplayCueTags.AddTag(WizardSkills::ShieldCueTag());
}
UWizardSkillHealEffect::UWizardSkillHealEffect() { AddValueModifier(this, UWizardAttributeSet::GetHealthAttribute(), EGameplayModOp::Additive); }
UWizardSkillShieldEffect::UWizardSkillShieldEffect() { AddValueModifier(this, UWizardAttributeSet::GetShieldAttribute(), EGameplayModOp::Override); }
UWizardSkillSlowEffect::UWizardSkillSlowEffect()
{
	FGameplayEffectCue& Cue = GameplayCues.AddDefaulted_GetRef();
	Cue.GameplayCueTags.AddTag(WizardCombat::SlowTag());
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(1.f);
	AddValueModifier(this, UWizardAttributeSet::GetMoveSpeedAttribute(), EGameplayModOp::Multiplicitive);
	// 同类减速刷新而不叠乘；效果到期由GAS准确撤销。
	StackingType = EGameplayEffectStackingType::AggregateByTarget;
	StackLimitCount = 1;
	StackDurationRefreshPolicy = EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;
}
UWizardSkillDamageEffect::UWizardSkillDamageEffect()
{
	FGameplayEffectExecutionDefinition& Execution = Executions.AddDefaulted_GetRef();
	Execution.CalculationClass = UWizardDamageExecution::StaticClass();
}

void WizardSkills::ApplyAreaDamage(UAbilitySystemComponent* Source, const UWizardSkillDefinition* Definition, const FVector& Center, FGameplayEffectContextHandle Context)
{
	if (!Source || !Definition || !Source->GetWorld()) return;
	AActor* Avatar = Source->GetAvatarActor();
	TArray<FOverlapResult> Hits;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(SkillArea), false, Avatar);
	FCollisionObjectQueryParams Objects(ECC_Pawn);
	Source->GetWorld()->OverlapMultiByObjectType(Hits, Center, FQuat::Identity, Objects, FCollisionShape::MakeSphere(Definition->Radius), Query);
	// 遮挡查询忽略全部敌人，避免前排替后排挡伤害，也避免死亡移除碰撞改变遍历结果。
	for (TActorIterator<AWizardEnemy> It(Source->GetWorld()); It; ++It) Query.AddIgnoredActor(*It);
	TSet<AActor*> Damaged;
	for (const FOverlapResult& Hit : Hits)
	{
		AWizardEnemy* Enemy = Cast<AWizardEnemy>(Hit.GetActor());
		if (!Enemy || Enemy->IsDead() || Damaged.Contains(Enemy)) continue;
		FHitResult Obstacle;
		const bool bBlocked = Source->GetWorld()->LineTraceSingleByChannel(Obstacle, Center + FVector(0,0,30), Enemy->GetActorLocation(), ECC_Visibility, Query);
		if (bBlocked && Obstacle.GetActor() != Enemy) continue;
		Damaged.Add(Enemy);
		FGameplayEffectSpecHandle Damage = Source->MakeOutgoingSpec(UWizardSkillDamageEffect::StaticClass(), 1.f, Context);
		Damage.Data->SetSetByCallerMagnitude(WizardSkills::DamageScaleTag(), Definition->Power);
		Source->ApplyGameplayEffectSpecToTarget(*Damage.Data.Get(), Enemy->GetAbilitySystemComponent());
	}
}
