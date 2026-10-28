// Wizard 项目：攻击蒙太奇的出手时机桥接到 GAS。
#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_WizardBasicAttackFire.generated.h"

UCLASS(meta=(DisplayName="Wizard Basic Attack Fire"))
class WIZARD_API UAnimNotify_WizardBasicAttackFire : public UAnimNotify
{
	GENERATED_BODY()
public:
	UAnimNotify_WizardBasicAttackFire();
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
