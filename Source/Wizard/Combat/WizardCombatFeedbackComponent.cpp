#include "Combat/WizardCombatFeedbackComponent.h"
#include "WizardPlayerController.h"
#include "WizardPlayerState.h"
#include "UI/WizardUIRoot.h"
#include "GAS/WizardAttributeSet.h"
#include "Camera/CameraShakeBase.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/World.h"
#include "Misc/CoreDelegates.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "UObject/ConstructorHelpers.h"
#include "Combat/WizardImpactShake.h"
UWizardCombatFeedbackComponent::UWizardCombatFeedbackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bTickEvenWhenPaused = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	static ConstructorHelpers::FClassFinder<UWizardCombatFeedbackWidget> FeedbackWidget(TEXT("/Game/Wizard/UI/WBP_CombatFeedback"));
	WidgetClass = FeedbackWidget.Class;
}
void UWizardCombatFeedbackComponent::BeginPlay()
{
	Super::BeginPlay();
	// 首次弹丸可能立即销毁，先完成Cue加载，避免首击被异步资源请求吞掉。
	UAbilitySystemGlobals::Get().GetGameplayCueManager()->FinishLoadingGameplayCueNotifies();
	LastRealTime = GetWorld()->GetRealTimeSeconds();
	DeactivateHandle = FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this, &ThisClass::OnDeactivate);
	ActivateHandle = FCoreDelegates::ApplicationHasReactivatedDelegate.AddUObject(this, &ThisClass::OnActivate);
}
void UWizardCombatFeedbackComponent::OnDeactivate()
{
	bApplicationActive = false;
	ResetFeedback();
}
void UWizardCombatFeedbackComponent::OnActivate()
{
	bApplicationActive = true;
	LastRealTime = GetWorld()->GetRealTimeSeconds();
}
void UWizardCombatFeedbackComponent::RestoreTime()
{
	if (bOwnsDilation && GetWorld())
		GetWorld()->GetWorldSettings()->SetTimeDilation(OriginalDilation);
	bOwnsDilation = false;
	StopEnd = 0;
}
void UWizardCombatFeedbackComponent::ResetFeedback()
{
	RestoreTime();
	PendingStop = 0;
	PendingLevel = 0;
	Numbers.Reset();
	HurtOpacity = 0;
	if (auto* PC = Cast<APlayerController>(GetOwner()))
		if (PC->PlayerCameraManager && ActiveShake)
			PC->PlayerCameraManager->StopCameraShake(ActiveShake, true);
	ActiveShake = nullptr;
	NextImpact = GetWorld() ? GetWorld()->GetRealTimeSeconds() + .12 : 0;
}
void UWizardCombatFeedbackComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	ResetFeedback();
	FCoreDelegates::ApplicationWillDeactivateDelegate.Remove(DeactivateHandle);
	FCoreDelegates::ApplicationHasReactivatedDelegate.Remove(ActivateHandle);
	if (Widget)
		Widget->RemoveFromParent();
	Super::EndPlay(Reason);
}
void UWizardCombatFeedbackComponent::RequestImpact(float Seconds, int32 Level)
{
	if (!GetWorld() || bOwnsDilation || GetWorld()->GetRealTimeSeconds() < NextImpact)
		return;
	PendingStop = FMath::Max(PendingStop, FMath::Clamp(Seconds, 0.f, .08f));
	PendingLevel = FMath::Max(PendingLevel, FMath::Clamp(Level, 0, 3));
}
void UWizardCombatFeedbackComponent::AddNumber(const FWizardCombatResult& R, float Amount, int32 Kind)
{
	if (Amount <= 0)
		return;
	const double Now = GetWorld()->GetRealTimeSeconds();
	for (auto& N : Numbers)
	{
		if (N.Target == R.Target && N.Kind == Kind && Now - N.Born < .1)
		{
			N.Amount += Amount;
			return;
		}
	}
	if (Numbers.Num() >= 32)
		Numbers.RemoveAt(0);
	FWizardFloatingNumber N;
	N.Target = R.Target;
	N.Position = R.Location + FVector(0, 0, 85);
	N.Amount = Amount;
	N.Kind = Kind;
	N.Born = Now;
	Numbers.Add(N);
}
void UWizardCombatFeedbackComponent::Consume(const FWizardCombatResult& R)
{
	auto* PC = Cast<AWizardPlayerController>(GetOwner());
	if (!bApplicationActive || !PC || !PC->IsLocalController() || PC->IsMenuOpen() || GetWorld()->IsPaused())
		return;
	const bool bPlayer = R.Target == PC->GetPawn();
	AddNumber(R, R.HealthLost, bPlayer ? 1 : 0);
	AddNumber(R, R.Absorbed, 2);
	AddNumber(R, R.Healed, 3);
	if (bPlayer && R.HealthLost > 0)
		HurtOpacity = .45f;
	if (bPlayer && R.bKilled)
	{
		RestoreTime();
		PendingStop = 0;
		PendingLevel = 0;
		return;
	}
	if (R.HealthLost <= 0 && R.Absorbed <= 0)
		return;
	float Stop = R.Presentation ? R.Presentation->HitStopSeconds : 0;
	int32 Level = R.Presentation ? R.Presentation->ImpactLevel : 1;
	if (R.bKilled)
	{
		Stop = FMath::Max(Stop, .045f);
		Level = FMath::Max(Level, 2);
	}
	if (bPlayer && R.bShieldBroken)
	{
		Stop = FMath::Max(Stop, .04f);
		Level = 3;
	}
	RequestImpact(Stop, Level);
}
void UWizardCombatFeedbackComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function)
{
	Super::TickComponent(DeltaTime, TickType, Function);
	auto* PC = Cast<AWizardPlayerController>(GetOwner());
	if (!PC || !PC->IsLocalController())
		return;
	const double Now = GetWorld()->GetRealTimeSeconds();
	const float RealDelta = FMath::Clamp(static_cast<float>(Now - LastRealTime), 0.f, .2f);
	LastRealTime = Now;
	if (!Widget && PC->GetUIRoot() && WidgetClass)
	{
		Widget = CreateWidget<UWizardCombatFeedbackWidget>(PC, WidgetClass);
		if (Widget)
		{
			Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
			Widget->ForceVolatile(true);
			PC->GetUIRoot()->AddCombatFeedback(Widget);
		}
	}
	const auto* State = PC->GetPlayerState<AWizardPlayerState>();
	if (!bApplicationActive || PC->IsMenuOpen() || GetWorld()->IsPaused() || !PC->GetPawn())
	{
		ResetFeedback();
		return;
	}
	if (State && State->GetAttributes()->GetHealth() <= 0)
	{
		RestoreTime();
		PendingStop = 0;
		PendingLevel = 0;
		NextImpact = Now + .12;
		if (PC->PlayerCameraManager && ActiveShake)
			PC->PlayerCameraManager->StopCameraShake(ActiveShake, true);
		ActiveShake = nullptr;
	}
	Numbers.RemoveAll([Now](const auto& N) { return Now - N.Born > .85; });
	HurtOpacity = FMath::Max(0.f, HurtOpacity - RealDelta * 2);
	if (bOwnsDilation && Now >= StopEnd)
	{
		RestoreTime();
		NextImpact = Now + .12;
	}
	if (!bOwnsDilation && Now >= NextImpact && (PendingLevel > 0 || PendingStop > 0))
	{
		if (bShakeEnabled && PendingLevel > 0 && PC->PlayerCameraManager)
		{
			if (ActiveShake)
				PC->PlayerCameraManager->StopCameraShake(ActiveShake, true);
			const TSubclassOf<UCameraShakeBase> Shake = PendingLevel >= 3   ? UWizardHeavyImpactShake::StaticClass()
			                                            : PendingLevel == 2 ? UWizardMediumImpactShake::StaticClass()
			                                                                : UWizardLightImpactShake::StaticClass();
			ActiveShake = PC->PlayerCameraManager->StartCameraShake(Shake, Intensity);
		}
		if (bHitStopEnabled && PendingStop > 0)
		{
			OriginalDilation = GetWorld()->GetWorldSettings()->TimeDilation;
			GetWorld()->GetWorldSettings()->SetTimeDilation(OriginalDilation * .1f);
			bOwnsDilation = true;
			StopEnd = Now + PendingStop;
		}
		else
			NextImpact = Now + .12;
	}
	PendingStop = 0;
	PendingLevel = 0;
}
int32 UWizardCombatFeedbackWidget::NativePaint(const FPaintArgs& Args, const FGeometry& G, const FSlateRect& Clip,
                                               FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool Enabled) const
{
	const int32 Base = Super::NativePaint(Args, G, Clip, Out, Layer, Style, Enabled);
	auto* PC = Cast<AWizardPlayerController>(GetOwningPlayer());
	if (!PC || !PC->CombatFeedback)
		return Base;
	const auto* Feedback = PC->CombatFeedback.Get();
	const double Now = GetWorld()->GetRealTimeSeconds();
	const auto Font = FCoreStyle::GetDefaultFontStyle("Bold", FontSize);
	for (const auto& N : Feedback->GetNumbers())
	{
		FVector2D Position;
		if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, N.Position, Position, true))
			continue;
		const float Age = static_cast<float>(Now - N.Born);
		Position -= FVector2D(18, Age * 65 + N.Kind * 19);
		FLinearColor Color = N.Kind == 1 ? PlayerDamageColor : N.Kind == 2 ? ShieldColor : N.Kind == 3 ? HealColor : DamageColor;
		Color.A *= FMath::Clamp((.85f - Age) / .25f, 0.f, 1.f);
		const FString Prefix = N.Kind == 2 ? TEXT("[") : N.Kind == 3 ? TEXT("+") : TEXT("-");
		const FString Text = Prefix + FString::SanitizeFloat(N.Amount, 0) + (N.Kind == 2 ? TEXT("]") : TEXT(""));
		FSlateDrawElement::MakeText(Out, Base + 1, G.ToPaintGeometry(FVector2D(180, 40), FSlateLayoutTransform(Position + FVector2D(1, 2))),
		                            Text, Font, ESlateDrawEffect::None, FLinearColor(0, 0, 0, Color.A));
		FSlateDrawElement::MakeText(Out, Base + 2, G.ToPaintGeometry(FVector2D(180, 40), FSlateLayoutTransform(Position)), Text, Font,
		                            ESlateDrawEffect::None, Color);
	}
	const FVector2D Size = G.GetLocalSize();
	const FLinearColor Edge(1, .04f, .02f, Feedback->GetHurtOpacity());
	const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
	for (int32 I = 0; I < 4; ++I)
	{
		const FVector2D P = I == 1 ? FVector2D(Size.X - 8, 0) : I == 3 ? FVector2D(0, Size.Y - 8) : FVector2D::ZeroVector;
		const FVector2D S = I < 2 ? FVector2D(8, Size.Y) : FVector2D(Size.X, 8);
		FSlateDrawElement::MakeBox(Out, Base + 1, G.ToPaintGeometry(S, FSlateLayoutTransform(P)), Brush, ESlateDrawEffect::None, Edge);
	}
	return Base + 2;
}
