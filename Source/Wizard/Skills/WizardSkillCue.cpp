#include "Skills/WizardSkillCue.h"
#include "Skills/WizardSkillDefinition.h"
#include "NativeGameplayTags.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SkillImpactCue, "GameplayCue.Wizard.Skill.Impact");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SkillShieldCue, "GameplayCue.Wizard.Skill.Shield");
FGameplayTag WizardSkills::ImpactCueTag() { return TAG_SkillImpactCue; }
FGameplayTag WizardSkills::ShieldCueTag() { return TAG_SkillShieldCue; }
void WizardSkills::PlayCastSound(AActor* Avatar, const UWizardSkillDefinition* Definition)
{
	if (Avatar && Definition && Definition->CastSound) UGameplayStatics::PlaySoundAtLocation(Avatar, Definition->CastSound, Avatar->GetActorLocation(), .6f);
}
UWizardSkillImpactCue::UWizardSkillImpactCue() { GameplayCueTag = WizardSkills::ImpactCueTag(); }
bool UWizardSkillImpactCue::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	const UWizardSkillDefinition* Definition = Cast<UWizardSkillDefinition>(Parameters.SourceObject.Get());
	if (!MyTarget || !Definition) return false;
	UFXSystemComponent* Spawned = nullptr;
	if (Definition->ImpactFX)
	{
		Spawned = UNiagaraFunctionLibrary::SpawnSystemAtLocation(MyTarget, Definition->ImpactFX, Parameters.Location, FRotator::ZeroRotator, Definition->EffectScale, true, true);
	}
	else if (Definition->CascadeFX)
	{
		Spawned = UGameplayStatics::SpawnEmitterAtLocation(MyTarget, Definition->CascadeFX, Parameters.Location, FRotator::ZeroRotator, Definition->EffectScale, true);
	}
	if (Spawned)
	{
		// 循环候选也有硬寿命；回调仅弱持有组件，换关卡由World清理定时器。
		FTimerHandle Expiry;
		TWeakObjectPtr<UFXSystemComponent> Weak = Spawned;
		MyTarget->GetWorld()->GetTimerManager().SetTimer(Expiry, [Weak]() { if (Weak.IsValid()) Weak->DestroyComponent(); }, Definition->EffectLifetime, false);
	}
	if (Definition->ImpactSound) UGameplayStatics::PlaySoundAtLocation(MyTarget, Definition->ImpactSound, Parameters.Location, .6f);
	return true;
}
AWizardSkillShieldCue::AWizardSkillShieldCue()
{
	GameplayCueTag = WizardSkills::ShieldCueTag();
	bAutoDestroyOnRemove = true;
}
bool AWizardSkillShieldCue::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	const UWizardSkillDefinition* Definition = Cast<UWizardSkillDefinition>(Parameters.SourceObject.Get());
	if (!MyTarget || !Definition || !Definition->ImpactFX) return false;
	if (!Effect)
	{
		Effect = UNiagaraFunctionLibrary::SpawnSystemAttached(Definition->ImpactFX, MyTarget->GetRootComponent(), NAME_None, FVector(0,0,-90), FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false);
		if (Effect) Effect->SetRelativeScale3D(Definition->EffectScale);
	}
	return true;
}
bool AWizardSkillShieldCue::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (Effect) { Effect->DeactivateImmediate(); Effect->DestroyComponent(); Effect = nullptr; }
	return true;
}
