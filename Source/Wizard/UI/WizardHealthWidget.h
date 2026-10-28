#pragma once
#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "WizardHealthWidget.generated.h"
class UAbilitySystemComponent;
class UTextBlock;
class AWizardPlayerState;
UCLASS()
class WIZARD_API UWizardHealthWidget : public UCommonUserWidget
{
	GENERATED_BODY()
public:
	void BindPlayer(AWizardPlayerState* State);
	void BindTarget(UAbilitySystemComponent* Target);
	float GetDisplayedPlayerHealth() const { return DisplayedPlayerHealth; }
	float GetDisplayedTargetHealth() const { return DisplayedTargetHealth; }
protected:
	virtual void NativeDestruct() override;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> PlayerHealth;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> TargetHealth;
private:
	void BindAttributes(UAbilitySystemComponent* Component, bool bPlayer);
	void UnbindAttributes(bool bPlayer);
	void Refresh(bool bPlayer);
	TWeakObjectPtr<UAbilitySystemComponent> PlayerASC;
	TWeakObjectPtr<UAbilitySystemComponent> TargetASC;
	FDelegateHandle PlayerHealthHandle, PlayerMaxHandle, TargetHealthHandle, TargetMaxHandle;
	float DisplayedPlayerHealth = 0.f;
	float DisplayedTargetHealth = 0.f;
};
