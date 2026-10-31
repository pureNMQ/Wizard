#include "UI/WizardPlayerStatusWidget.h"
#include "WizardPlayerState.h"
#include "GAS/WizardAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

void UWizardPlayerStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();
	APlayerController* Controller = GetOwningPlayer();
	BindPlayer(Controller ? Controller->GetPlayerState<AWizardPlayerState>() : nullptr);
}

void UWizardPlayerStatusWidget::BindPlayer(AWizardPlayerState* State)
{
	UnbindAttributes();
	PlayerState = State;
	if (State) GoldHandle = State->OnGoldChanged.AddUObject(this, &ThisClass::Refresh);
	UAbilitySystemComponent* Component = State ? State->GetAbilitySystemComponent() : nullptr;
	PlayerASC = Component;
	if (Component)
	{
		auto Bind = [this, Component](const FGameplayAttribute& Attribute)
		{
			return Component->GetGameplayAttributeValueChangeDelegate(Attribute).AddWeakLambda(this,
				[this](const FOnAttributeChangeData&) { Refresh(); });
		};
		HealthHandle = Bind(UWizardAttributeSet::GetHealthAttribute());
		MaxHealthHandle = Bind(UWizardAttributeSet::GetMaxHealthAttribute());
		ManaHandle = Bind(UWizardAttributeSet::GetManaAttribute());
		MaxManaHandle = Bind(UWizardAttributeSet::GetMaxManaAttribute());
	}
	Refresh();
}

void UWizardPlayerStatusWidget::UnbindAttributes()
{
	if (AWizardPlayerState* State = PlayerState.Get()) State->OnGoldChanged.Remove(GoldHandle);
	GoldHandle.Reset();
	PlayerState.Reset();
	if (UAbilitySystemComponent* Component = PlayerASC.Get())
	{
		Component->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetHealthAttribute()).Remove(HealthHandle);
		Component->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetMaxHealthAttribute()).Remove(MaxHealthHandle);
		Component->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetManaAttribute()).Remove(ManaHandle);
		Component->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetMaxManaAttribute()).Remove(MaxManaHandle);
	}
	HealthHandle.Reset();
	MaxHealthHandle.Reset();
	ManaHandle.Reset();
	MaxManaHandle.Reset();
	PlayerASC.Reset();
}

void UWizardPlayerStatusWidget::Refresh()
{
	UAbilitySystemComponent* Component = PlayerASC.Get();
	const float Health = Component ? Component->GetNumericAttribute(UWizardAttributeSet::GetHealthAttribute()) : 0.f;
	const float MaxHealth = Component ? Component->GetNumericAttribute(UWizardAttributeSet::GetMaxHealthAttribute()) : 0.f;
	const float Mana = Component ? Component->GetNumericAttribute(UWizardAttributeSet::GetManaAttribute()) : 0.f;
	const float MaxMana = Component ? Component->GetNumericAttribute(UWizardAttributeSet::GetMaxManaAttribute()) : 0.f;
	DisplayedPlayerHealth = Health;
	if (GoldText) GoldText->SetText(FText::AsNumber(PlayerState.IsValid() ? PlayerState->GetGold() : 0));
	if (HealthBar) HealthBar->SetPercent(MaxHealth > 0.f ? FMath::Clamp(Health / MaxHealth, 0.f, 1.f) : 0.f);
	if (ManaBar) ManaBar->SetPercent(MaxMana > 0.f ? FMath::Clamp(Mana / MaxMana, 0.f, 1.f) : 0.f);
	if (HealthText) HealthText->SetText(FText::FromString(FString::Printf(TEXT("%.0f/%.0f"), Health, MaxHealth)));
	if (ManaText) ManaText->SetText(FText::FromString(FString::Printf(TEXT("%.0f/%.0f"), Mana, MaxMana)));
}

void UWizardPlayerStatusWidget::NativeDestruct()
{
	UnbindAttributes();
	Super::NativeDestruct();
}
