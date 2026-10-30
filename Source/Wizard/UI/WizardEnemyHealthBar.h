// Wizard 项目：怪物血条只读取 GAS 属性，布局保留在 Widget Blueprint。
#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WizardEnemyHealthBar.generated.h"

class AWizardEnemy;
class UAbilitySystemComponent;
class UProgressBar;

UCLASS()
class WIZARD_API UWizardEnemyHealthBar : public UUserWidget
{
	GENERATED_BODY()
public:
	// 更换来源时解除旧订阅；空来源及死亡目标隐藏血条。
	UFUNCTION(BlueprintCallable, Category="Combat") void BindEnemy(AWizardEnemy* Enemy);
	UFUNCTION(BlueprintPure, Category="Combat") float GetHealthFraction() const { return HealthFraction; }
protected:
	virtual void NativeDestruct() override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> HealthBar;
private:
	friend class FWizardEnemyHealthBarTest;
	void UnbindAttributes();
	void Refresh();
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	FDelegateHandle HealthHandle;
	FDelegateHandle MaxHealthHandle;
	float HealthFraction = 0.f;
};
