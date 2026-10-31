#pragma once
#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "WizardPlayerStatusWidget.generated.h"
class UAbilitySystemComponent;
class UProgressBar;
class UTextBlock;
class AWizardPlayerState;

// 玩家状态模块；聚合 HUD 仅负责布局。
UCLASS()
class WIZARD_API UWizardPlayerStatusWidget : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	// 切换来源时先解除旧委托；空来源清空展示。
	void BindPlayer(AWizardPlayerState* State);
	float GetDisplayedPlayerHealth() const { return DisplayedPlayerHealth; }
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> HealthBar;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UProgressBar> ManaBar;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> HealthText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ManaText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> GoldText;
private:
	friend class FWizardPlayerStatusTest;
	void UnbindAttributes();
	void Refresh();
	TWeakObjectPtr<AWizardPlayerState> PlayerState;
	TWeakObjectPtr<UAbilitySystemComponent> PlayerASC;
	FDelegateHandle GoldHandle;
	FDelegateHandle HealthHandle, MaxHealthHandle, ManaHandle, MaxManaHandle;
	float DisplayedPlayerHealth = 0.f;
};
