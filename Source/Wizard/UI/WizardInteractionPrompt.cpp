#include "UI/WizardInteractionPrompt.h"
#include "Interaction/WizardInteractionComponent.h"
#include "Interaction/WizardInteractionTarget.h"
#include "WizardPlayerState.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"

void UWizardInteractionPrompt::NativeConstruct()
{
	Super::NativeConstruct();
	const AWizardPlayerState* State = GetOwningPlayerState<AWizardPlayerState>();
	Interaction = State ? State->GetInteraction() : nullptr;
	if (Interaction.IsValid())
	{
		PromptHandle = Interaction->OnPromptChanged.AddUObject(this, &ThisClass::RefreshPrompt);
		RefreshPrompt(Interaction->GetPrompt());
	}
	else PromptFrame->SetVisibility(ESlateVisibility::Hidden);
}
void UWizardInteractionPrompt::RefreshPrompt(const FText& Prompt)
{
	Target = Interaction.IsValid() ? Interaction->GetCurrentTarget() : nullptr;
	PromptText->SetText(Prompt);
	// 等下一次投影成功再显示，避免目标切换时在旧位置闪现。
	PromptFrame->SetVisibility(ESlateVisibility::Hidden);
}
void UWizardInteractionPrompt::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	APlayerController* Controller = GetOwningPlayer();
	if (!Target.IsValid() || PromptText->GetText().IsEmpty() || !Controller)
	{
		PromptFrame->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	// 俯视角投影整个模型包围盒，避免仅投影世界顶部中心时提示仍压在模型上。
	const FBox ModelBounds = Target->GetPromptWorldBounds();
	FVector2D Minimum(TNumericLimits<float>::Max(), TNumericLimits<float>::Max());
	FVector2D Maximum(-TNumericLimits<float>::Max(), -TNumericLimits<float>::Max());
	for (int32 Corner = 0; Corner < 8; ++Corner)
	{
		const FVector WorldPoint(Corner & 1 ? ModelBounds.Max.X : ModelBounds.Min.X,
			Corner & 2 ? ModelBounds.Max.Y : ModelBounds.Min.Y, Corner & 4 ? ModelBounds.Max.Z : ModelBounds.Min.Z);
		FVector2D ScreenPoint;
		if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(Controller, WorldPoint, ScreenPoint, true))
		{
			PromptFrame->SetVisibility(ESlateVisibility::Hidden);
			return;
		}
		Minimum = FVector2D(FMath::Min(Minimum.X, ScreenPoint.X), FMath::Min(Minimum.Y, ScreenPoint.Y));
		Maximum = FVector2D(FMath::Max(Maximum.X, ScreenPoint.X), FMath::Max(Maximum.Y, ScreenPoint.Y));
	}
	FVector2D Position((Minimum.X + Maximum.X) * .5f, Minimum.Y - 8.f);
	// 仅投影事件选出的目标，不做交互搜索或遮挡查询；适配镜头移动、窗口尺寸和DPI。
	Position = Geometry.AbsoluteToLocal(UWidgetLayoutLibrary::GetPlayerScreenWidgetGeometry(Controller).LocalToAbsolute(Position));
	const FVector2D Size = Geometry.GetLocalSize();
	if (Position.X < 0.f || Position.X > Size.X || Position.Y < 0.f || Position.Y > Size.Y)
	{
		PromptFrame->SetVisibility(ESlateVisibility::Hidden);
		return;
	}
	if (UCanvasPanelSlot* PromptSlot = Cast<UCanvasPanelSlot>(PromptFrame->Slot))
	{
		PromptSlot->SetPosition(Position);
		PromptFrame->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}
void UWizardInteractionPrompt::NativeDestruct()
{
	if (Interaction.IsValid()) Interaction->OnPromptChanged.Remove(PromptHandle);
	Interaction.Reset();
	Target.Reset();
	Super::NativeDestruct();
}
