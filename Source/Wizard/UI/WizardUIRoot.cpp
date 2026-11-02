#include "UI/WizardUIRoot.h"
#include "Components/OverlaySlot.h"
#include "Blueprint/UserWidget.h"
#include "WizardPlayerController.h"
#include "CommonActivatableWidget.h"
#include "Components/PanelWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"
#include "UI/WizardChallengeBanner.h"
#include "UI/WizardShopWidgets.h"
#include "Trading/WizardShopActor.h"
#include "WizardPlayerState.h"

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
	if (ChallengeBanner) ChallengeBanner->RemoveFromParent();
	HUDLayer->ClearChildren();
	UPanelSlot* HUDSlot = HUDLayer->AddChild(Widget);
	if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(HUDSlot))
	{
		OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		OverlaySlot->SetVerticalAlignment(VAlign_Fill);
	}
	AttachChallengeBanner();
	return HUDSlot != nullptr;
}

void UWizardUIRoot::AttachChallengeBanner()
{
	if (!HUDLayer || !ChallengeBanner || ChallengeBanner->GetParent() == HUDLayer) return;
	if (UOverlaySlot* BannerSlot = Cast<UOverlaySlot>(HUDLayer->AddChild(ChallengeBanner)))
	{
		BannerSlot->SetHorizontalAlignment(HAlign_Fill);
		BannerSlot->SetVerticalAlignment(VAlign_Fill);
	}
}

void UWizardUIRoot::ShowChallengeAnnouncement(FText Message, FLinearColor Color, float Duration)
{
	ShowChallengeAnnouncementDetail(Message, FText::GetEmpty(), Color, Duration);
}

void UWizardUIRoot::ShowChallengeAnnouncementDetail(FText Message, FText Detail, FLinearColor Color, float Duration)
{
	if (!HUDLayer) return;
	if (!ChallengeBanner)
	{
		UClass* BannerClass = ChallengeBannerClass.Get();
		if (!BannerClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("挑战提示布局未配置：WBP_ChallengeBanner"));
			return;
		}
		ChallengeBanner = CreateWidget<UWizardChallengeBanner>(GetOwningPlayer(), BannerClass);
	}
	AttachChallengeBanner();
	if (ChallengeBanner) ChallengeBanner->ShowAnnouncement(Message, Detail, Color, Duration);
}

bool UWizardUIRoot::HasOpenScreens() const
{
	return (MenuLayer && MenuLayer->GetActiveWidget()) || (ModalLayer && ModalLayer->GetActiveWidget());
}

bool UWizardUIRoot::OpenShop(AWizardShopActor* Shop)
{
	const AWizardPlayerController* Controller = Cast<AWizardPlayerController>(GetOwningPlayer());
	AWizardPlayerState* State = Controller ? Controller->GetPlayerState<AWizardPlayerState>() : nullptr;
	if (!IsValid(Shop) || HasOpenScreens() || !Shop->CanTrade(State)) return false;
	UWizardShopMenu* Menu = Cast<UWizardShopMenu>(PushMenu(Shop->ShopMenuClass));
	if (!Menu) return false;
	Menu->Configure(Shop);
	return Menu->IsActivated();
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
	if (ChallengeBanner) ChallengeBanner->RemoveFromParent();
	ChallengeBanner = nullptr;
	if (MenuLayer) MenuLayer->OnDisplayedWidgetChanged().RemoveAll(this);
	if (ModalLayer) ModalLayer->OnDisplayedWidgetChanged().RemoveAll(this);
	if (ModalLayer) ModalLayer->ClearWidgets();
	if (MenuLayer) MenuLayer->ClearWidgets();
	if (AWizardPlayerController* Controller = Cast<AWizardPlayerController>(GetOwningPlayer())) Controller->SetRootScreensOpen(false);
	Super::NativeDestruct();
}

void UWizardUIRoot::AddCombatFeedback(UUserWidget* Widget)
{
	if (HUDLayer && Widget)
	{
		UPanelSlot* Added = HUDLayer->AddChild(Widget);
		if (UOverlaySlot* Overlay = Cast<UOverlaySlot>(Added)) { Overlay->SetHorizontalAlignment(HAlign_Fill); Overlay->SetVerticalAlignment(VAlign_Fill); }
	}
}
