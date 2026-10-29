#include "UI/WizardPauseMenu.h"
#include "WizardPlayerController.h"

UWizardPauseMenu::UWizardPauseMenu()
{
	bIsBackHandler = true;
	bIsModal = true;
	SetIsFocusable(true);
}
TOptional<FUIInputConfig> UWizardPauseMenu::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, EMouseLockMode::DoNotLock, false);
}
bool UWizardPauseMenu::NativeOnHandleBackAction()
{
	DeactivateWidget();
	return true;
}
