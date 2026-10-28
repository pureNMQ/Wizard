#include "UI/WizardHealthWidget.h"
#include "WizardPlayerState.h"
#include "GAS/WizardAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/TextBlock.h"

void UWizardHealthWidget::BindPlayer(AWizardPlayerState* State)
{
	BindAttributes(State ? State->GetAbilitySystemComponent() : nullptr, true);
}
void UWizardHealthWidget::BindTarget(UAbilitySystemComponent* Target)
{
	BindAttributes(Target, false);
}
void UWizardHealthWidget::BindAttributes(UAbilitySystemComponent* Component, bool bPlayer)
{
	TWeakObjectPtr<UAbilitySystemComponent>& Bound = bPlayer ? PlayerASC : TargetASC;
	if (Bound.Get() == Component) return;
	UnbindAttributes(bPlayer);
	Bound = Component;
	if (Component)
	{
		FDelegateHandle& HealthHandle = bPlayer ? PlayerHealthHandle : TargetHealthHandle;
		FDelegateHandle& MaxHandle = bPlayer ? PlayerMaxHandle : TargetMaxHandle;
		HealthHandle = Component->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetHealthAttribute()).AddWeakLambda(this,
			[this, bPlayer](const FOnAttributeChangeData&) { Refresh(bPlayer); });
		MaxHandle = Component->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetMaxHealthAttribute()).AddWeakLambda(this,
			[this, bPlayer](const FOnAttributeChangeData&) { Refresh(bPlayer); });
	}
	Refresh(bPlayer);
}
void UWizardHealthWidget::UnbindAttributes(bool bPlayer)
{
	TWeakObjectPtr<UAbilitySystemComponent>& Bound = bPlayer ? PlayerASC : TargetASC;
	FDelegateHandle& HealthHandle = bPlayer ? PlayerHealthHandle : TargetHealthHandle;
	FDelegateHandle& MaxHandle = bPlayer ? PlayerMaxHandle : TargetMaxHandle;
	if (UAbilitySystemComponent* Component = Bound.Get())
	{
		Component->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetHealthAttribute()).Remove(HealthHandle);
		Component->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetMaxHealthAttribute()).Remove(MaxHandle);
	}
	HealthHandle.Reset();
	MaxHandle.Reset();
	Bound.Reset();
}
void UWizardHealthWidget::Refresh(bool bPlayer)
{
	UAbilitySystemComponent* Component = (bPlayer ? PlayerASC : TargetASC).Get();
	const float Health = Component ? Component->GetNumericAttribute(UWizardAttributeSet::GetHealthAttribute()) : 0.f;
	const float Maximum = Component ? Component->GetNumericAttribute(UWizardAttributeSet::GetMaxHealthAttribute()) : 0.f;
	(bPlayer ? DisplayedPlayerHealth : DisplayedTargetHealth) = Health;
	if (UTextBlock* Label = bPlayer ? PlayerHealth : TargetHealth)
	{
		Label->SetText(FText::FromString(FString::Printf(TEXT("%s HP  %.0f / %.0f"), bPlayer ? TEXT("Player") : TEXT("Target"), Health, Maximum)));
	}
}
void UWizardHealthWidget::NativeDestruct()
{
	UnbindAttributes(true);
	UnbindAttributes(false);
	Super::NativeDestruct();
}
