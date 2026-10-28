#pragma once
#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "WizardPauseMenu.generated.h"
UCLASS()
class WIZARD_API UWizardPauseMenu : public UCommonActivatableWidget
{
	GENERATED_BODY()
public:
	UWizardPauseMenu();
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual bool NativeOnHandleBackAction() override;
};
