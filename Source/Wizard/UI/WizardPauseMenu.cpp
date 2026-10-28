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
void UWizardPauseMenu::NativeOnActivated()
{
	Super::NativeOnActivated();
	if (AWizardPlayerController* Controller = Cast<AWizardPlayerController>(GetOwningPlayer())) Controller->SetMenuOpen(true);
}
void UWizardPauseMenu::NativeOnDeactivated()
{
	if (AWizardPlayerController* Controller = Cast<AWizardPlayerController>(GetOwningPlayer())) Controller->SetMenuOpen(false);
	Super::NativeOnDeactivated();
}
bool UWizardPauseMenu::NativeOnHandleBackAction()
{
	DeactivateWidget();
	return true;
}
