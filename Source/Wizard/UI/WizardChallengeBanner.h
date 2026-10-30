// Wizard 项目：挑战提示只管理显示状态；布局保存在 Widget Blueprint。
#pragma once
#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "Engine/TimerHandle.h"
#include "WizardChallengeBanner.generated.h"

class UTextBlock;

UCLASS()
class WIZARD_API UWizardChallengeBanner : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	// 替换当前提示并重置隐藏计时；不改变世界暂停或输入模式。
	UFUNCTION(BlueprintCallable, Category="Challenge") void ShowAnnouncement(FText Message, FText Detail, FLinearColor Color, float Duration);
protected:
	virtual void NativeDestruct() override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> AnnouncementText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> DetailText;
private:
	void HideAnnouncement();
	FTimerHandle HideTimer;
};
