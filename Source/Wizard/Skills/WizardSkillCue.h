#pragma once
#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "GameplayCueNotify_Actor.h"
#include "WizardSkillCue.generated.h"
class UWizardSkillDefinition;
class UNiagaraComponent;
namespace WizardSkills
{
	WIZARD_API FGameplayTag ImpactCueTag();
	WIZARD_API FGameplayTag ShieldCueTag();
	WIZARD_API void PlayCastSound(AActor* Avatar, const UWizardSkillDefinition* Definition);
}
UCLASS(Blueprintable)
class WIZARD_API UWizardSkillImpactCue : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
public:
	UWizardSkillImpactCue();
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};
UCLASS(Blueprintable)
class WIZARD_API AWizardSkillShieldCue : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()
public:
	AWizardSkillShieldCue();
	virtual bool OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) override;
private:
	UPROPERTY() TObjectPtr<UNiagaraComponent> Effect;
};
