#include "UI/WizardUIRoot.h"
#include "Blueprint/UserWidget.h"
#include "WizardPlayerController.h"
#include "CommonActivatableWidget.h"
#include "Components/PanelWidget.h"
#include "Components/OverlaySlot.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

void UWizardUIRoot::NativeConstruct()
{
	Super::NativeConstruct();
	if (!ensure(HUDLayer && MenuLayer && ModalLayer)) return;
	// 无转场空隙，暂停与输入状态随实际栈顶同步切换。
	MenuLayer->SetTransitionDuration(0.f);
	ModalLayer->SetTransitionDuration(0.f);
	MenuLayer->OnDisplayedWidgetChanged().AddUObject(this, &ThisClass::HandleDisplayedWidgetChanged);
	ModalLayer->OnDisplayedWidgetChanged().AddUObject(this, &ThisClass::HandleDisplayedWidgetChanged);
	RefreshScreenState();
}

bool UWizardUIRoot::SetHUD(UUserWidget* Widget)
{
	if (!HUDLayer || !Widget) return false;
	HUDLayer->ClearChildren();
	UPanelSlot* HUDSlot = HUDLayer->AddChild(Widget);
	if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(HUDSlot))
	{
		OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		OverlaySlot->SetVerticalAlignment(VAlign_Fill);
	}
	return HUDSlot != nullptr;
}

bool UWizardUIRoot::HasOpenScreens() const
{
	return (MenuLayer && MenuLayer->GetActiveWidget()) || (ModalLayer && ModalLayer->GetActiveWidget());
}

UCommonActivatableWidget* UWizardUIRoot::PushMenu(TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	if (!MenuLayer || !ModalLayer || ModalLayer->GetActiveWidget() || !WidgetClass) return nullptr;
	UCommonActivatableWidget* Widget = MenuLayer->AddWidget(WidgetClass);
	RefreshScreenState();
	return Widget;
}

UCommonActivatableWidget* UWizardUIRoot::PushModal(TSubclassOf<UCommonActivatableWidget> WidgetClass)
{
	if (!ModalLayer || !WidgetClass) return nullptr;
	UCommonActivatableWidget* Widget = ModalLayer->AddWidget(WidgetClass);
	RefreshScreenState();
	return Widget;
}

bool UWizardUIRoot::CloseTop()
{
	UCommonActivatableWidget* Top = ModalLayer ? ModalLayer->GetActiveWidget() : nullptr;
	if (!Top && MenuLayer) Top = MenuLayer->GetActiveWidget();
	if (!Top) return false;
	Top->DeactivateWidget();
	RefreshScreenState();
	return true;
}

void UWizardUIRoot::HandleDisplayedWidgetChanged(UCommonActivatableWidget* Widget)
{
	RefreshScreenState();
}

void UWizardUIRoot::RefreshScreenState()
{
	const bool bModalOpen = ModalLayer && ModalLayer->GetActiveWidget();
	// Slate 禁用 Menu 子树也阻止其 CommonUI 路由；关闭 Modal 后恢复原栈。
	if (MenuLayer) MenuLayer->SetIsEnabled(!bModalOpen);
	if (AWizardPlayerController* Controller = Cast<AWizardPlayerController>(GetOwningPlayer()))
	{
		Controller->SetRootScreensOpen(HasOpenScreens());
	}
}

void UWizardUIRoot::NativeDestruct()
{
	if (MenuLayer) MenuLayer->OnDisplayedWidgetChanged().RemoveAll(this);
	if (ModalLayer) ModalLayer->OnDisplayedWidgetChanged().RemoveAll(this);
	if (ModalLayer) ModalLayer->ClearWidgets();
	if (MenuLayer) MenuLayer->ClearWidgets();
	if (AWizardPlayerController* Controller = Cast<AWizardPlayerController>(GetOwningPlayer())) Controller->SetRootScreensOpen(false);
	Super::NativeDestruct();
}
