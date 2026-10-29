#pragma once
#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "WizardUIRoot.generated.h"
class UPanelWidget;
class UCommonActivatableWidget;
class UCommonActivatableWidgetStack;
class UUserWidget;

// 固定三层；布局由 WBP_UIRoot 提供，不开放层注册。
UCLASS()
class WIZARD_API UWizardUIRoot : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	bool SetHUD(UUserWidget* Widget);
	// Modal 存在时拒绝 Menu，防止较晚激活的 Menu 抢占输入路由。
	UFUNCTION(BlueprintCallable, Category="UI") UCommonActivatableWidget* PushMenu(TSubclassOf<UCommonActivatableWidget> WidgetClass);
	UFUNCTION(BlueprintCallable, Category="UI") UCommonActivatableWidget* PushModal(TSubclassOf<UCommonActivatableWidget> WidgetClass);
	UFUNCTION(BlueprintCallable, Category="UI") bool CloseTop();
	UFUNCTION(BlueprintPure, Category="UI") bool HasOpenScreens() const;
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UPanelWidget> HUDLayer;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UCommonActivatableWidgetStack> MenuLayer;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UCommonActivatableWidgetStack> ModalLayer;
private:
	void HandleDisplayedWidgetChanged(UCommonActivatableWidget* Widget);
	void RefreshScreenState();
};
