#include "UI/WizardEnemyHealthBar.h"
#include "Combat/WizardEnemy.h"
#include "GAS/WizardAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"

void UWizardEnemyHealthBar::BindEnemy(AWizardEnemy* Enemy)
{
	UnbindAttributes();
	UAbilitySystemComponent* ASC = Enemy ? Enemy->GetAbilitySystemComponent() : nullptr;
	SourceASC = ASC;
	if (ASC)
	{
		HealthHandle = ASC->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetHealthAttribute()).AddWeakLambda(this,
			[this](const FOnAttributeChangeData&) { Refresh(); });
		MaxHealthHandle = ASC->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetMaxHealthAttribute()).AddWeakLambda(this,
			[this](const FOnAttributeChangeData&) { Refresh(); });
	}
	Refresh();
}

void UWizardEnemyHealthBar::Refresh()
{
	const UAbilitySystemComponent* ASC = SourceASC.Get();
	const float Health = ASC ? ASC->GetNumericAttribute(UWizardAttributeSet::GetHealthAttribute()) : 0.f;
	const float MaxHealth = ASC ? ASC->GetNumericAttribute(UWizardAttributeSet::GetMaxHealthAttribute()) : 0.f;
	HealthFraction = MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f;
	if (HealthBar) HealthBar->SetPercent(HealthFraction);
	SetVisibility(Health > 0.f && MaxHealth > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UWizardEnemyHealthBar::UnbindAttributes()
{
	if (UAbilitySystemComponent* ASC = SourceASC.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetHealthAttribute()).Remove(HealthHandle);
		ASC->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetMaxHealthAttribute()).Remove(MaxHealthHandle);
	}
	HealthHandle.Reset();
	MaxHealthHandle.Reset();
	SourceASC.Reset();
}

void UWizardEnemyHealthBar::NativeDestruct()
{
	UnbindAttributes();
	Super::NativeDestruct();
}
