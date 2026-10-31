// Wizard 项目：技能和物品复用图标拖拽预览，业务请求保留在各自拖拽操作中。
#pragma once
#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "WizardIconDragPreview.generated.h"

class UImage;
class UDragDropOperation;
class APlayerController;

UCLASS()
class WIZARD_API UWizardIconDragPreview : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	void Configure(UTexture2D* Texture);
protected:
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UImage> Icon;
};

namespace WizardIconDrag
{
	// 只配置通用视觉，不读取或更改业务负载；布局和尺寸来自预览蓝图。
	WIZARD_API void SetVisual(UDragDropOperation* Operation, APlayerController* Player, TSubclassOf<UWizardIconDragPreview> PreviewClass, UTexture2D* Texture);
}
