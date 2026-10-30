#include "UI/WizardChallengeBanner.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"
#include "TimerManager.h"

void UWizardChallengeBanner::ShowAnnouncement(FText Message, FText Detail, FLinearColor Color, float Duration)
{
	if (AnnouncementText)
	{
		AnnouncementText->SetText(Message);
		AnnouncementText->SetColorAndOpacity(FSlateColor(Color));
	}
	if (DetailText)
	{
		DetailText->SetText(Detail);
		DetailText->SetVisibility(Detail.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(HideTimer);
		World->GetTimerManager().SetTimer(HideTimer, this, &ThisClass::HideAnnouncement, FMath::Max(.01f, Duration), false);
	}
}

void UWizardChallengeBanner::HideAnnouncement()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UWizardChallengeBanner::NativeDestruct()
{
	if (UWorld* World = GetWorld()) World->GetTimerManager().ClearTimer(HideTimer);
	SetVisibility(ESlateVisibility::Collapsed);
	Super::NativeDestruct();
}
