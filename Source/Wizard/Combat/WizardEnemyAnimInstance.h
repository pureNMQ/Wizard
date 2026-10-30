// Wizard 项目：动画蓝图从移动组件读取速度，动画接线保留在蓝图。
#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "WizardEnemyAnimInstance.generated.h"
UCLASS(Blueprintable)
class WIZARD_API UWizardEnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadOnly, Category="Enemy") float Speed = 0;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};
