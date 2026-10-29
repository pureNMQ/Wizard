// Wizard 项目：显示当前交互物上方的提示；布局与文字样式由Widget Blueprint配置。
#pragma once
#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "WizardInteractionPrompt.generated.h"

class UTextBlock;
class UBorder;
class UWizardInteractionComponent;
class UWizardInteractionTarget;

UCLASS()
class WIZARD_API UWizardInteractionPrompt : public UCommonUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> PromptText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UBorder> PromptFrame;
private:
	void RefreshPrompt(const FText& Prompt);
	TWeakObjectPtr<UWizardInteractionComponent> Interaction;
	TWeakObjectPtr<UWizardInteractionTarget> Target;
	FDelegateHandle PromptHandle;
};
