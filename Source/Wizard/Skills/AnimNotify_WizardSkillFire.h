#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_WizardSkillFire.generated.h"
UCLASS()
class WIZARD_API UAnimNotify_WizardSkillFire : public UAnimNotify
{
	GENERATED_BODY()
public:
	UAnimNotify_WizardSkillFire();
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
