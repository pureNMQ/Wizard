#include "UI/WizardSkillWidgets.h"
#include "Skills/WizardSkillLoadoutComponent.h"
#include "WizardPlayerState.h"
#include "WizardPlayerController.h"
#include "GAS/WizardAttributeSet.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/ScrollBox.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Input/Reply.h"

FText WizardSkills::ResultText(EWizardSkillResult Result)
{
	const TCHAR* Text = TEXT("无法施放");
	switch (Result)
	{
	case EWizardSkillResult::Success: Text = TEXT(""); break;
	case EWizardSkillResult::DuplicateSkill: Text = TEXT("该技能已经装备，请拖动槽位进行交换"); break;
	case EWizardSkillResult::NotAtAlchemy: Text = TEXT("需要靠近营地炼金台"); break;
	case EWizardSkillResult::NotEnoughMana: Text = TEXT("法力不足"); break;
	case EWizardSkillResult::CoolingDown: Text = TEXT("技能冷却中"); break;
	case EWizardSkillResult::Busy: Text = TEXT("当前无法施法"); break;
	case EWizardSkillResult::InvalidTarget: Text = TEXT("目标或落点无效"); break;
	case EWizardSkillResult::FullHealth: Text = TEXT("生命已满"); break;
	case EWizardSkillResult::Dead: Text = TEXT("角色已死亡"); break;
	case EWizardSkillResult::InvalidSlot: Text = TEXT("无效槽位"); break;
	case EWizardSkillResult::UnknownSkill: Text = TEXT("未知技能"); break;
	case EWizardSkillResult::InvalidConfiguration: Text = TEXT("技能配置无效"); break;
	default: break;
	}
	return FText::FromString(Text);
}
void UWizardSkillTooltip::Configure(const UWizardSkillDefinition* Definition)
{
	if (DescriptionText && Definition) DescriptionText->SetText(FText::FromString(FString::Printf(TEXT("%s\n%s\n法力 %.0f　冷却 %.1f 秒"), *Definition->DisplayName.ToString(), *Definition->Description.ToString(), Definition->ManaCost, Definition->Cooldown)));
}
void UWizardSkillTile::Configure(const UWizardSkillDefinition* InDefinition, int32 InSlot, UWizardSkillMenuController* InController)
{
	Definition = InDefinition;
	SlotIndex = InSlot;
	MenuController = InController;
	if (Icon)
	{
		Icon->SetBrushFromTexture(Definition ? Definition->Icon : nullptr);
		Icon->SetVisibility(Definition ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
	if (Title) Title->SetText(Definition ? Definition->DisplayName : FText::GetEmpty());
	SetHUDState(0.f, true);
	SetToolTip(nullptr);
	// 菜单卡片使用引擎悬浮提示；HUD与拖拽影像不创建详情，关闭后随卡片释放。
	if (InController && Definition && TooltipClass)
	{
		UWizardSkillTooltip* Tooltip = CreateWidget<UWizardSkillTooltip>(GetOwningPlayer(), TooltipClass);
		if (Tooltip) { Tooltip->Configure(Definition); SetToolTip(Tooltip); }
	}
}
void UWizardSkillTile::SetStatus(const FString& Status, bool bAvailable)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Status));
		StatusText->SetVisibility(Status.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
	if (Icon) Icon->SetColorAndOpacity(bAvailable ? FLinearColor::White : FLinearColor(.35f,.35f,.35f,1));
}
void UWizardSkillTile::SetHUDState(float Remaining, bool bEnoughMana)
{
	const bool bCoolingDown = FMath::IsFinite(Remaining) && Remaining > 0.f;
	// 进度只展示已有冷却状态，不影响技能提交或冷却时长。
	if (CooldownProgress)
	{
		const float Duration = Definition ? Definition->Cooldown : 0.f;
		CooldownProgress->SetPercent(bCoolingDown && Duration > 0.f ? FMath::Clamp(Remaining / Duration, 0.f, 1.f) : 0.f);
		CooldownProgress->SetVisibility(bCoolingDown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (!CooldownText) return;
	const FText Text = bCoolingDown ? FText::FromString(FString::Printf(TEXT("%.1f"), Remaining))
		: (bEnoughMana ? FText::GetEmpty() : FText::FromString(TEXT("法力\n不足")));
	CooldownText->SetText(Text);
	CooldownText->SetVisibility(bCoolingDown || !bEnoughMana ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}
FReply UWizardSkillTile::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (!MenuController.IsValid() || !Definition || !MenuController->CanDragSkill(Definition->SkillId, SlotIndex) || Event.GetEffectingButton() != EKeys::LeftMouseButton) return Super::NativeOnMouseButtonDown(Geometry, Event);
	return UWidgetBlueprintLibrary::DetectDragIfPressed(Event, this, EKeys::LeftMouseButton).NativeReply;
}
void UWizardSkillTile::NativeOnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event, UDragDropOperation*& Operation)
{
	if (!MenuController.IsValid() || !Definition || !MenuController->CanDragSkill(Definition->SkillId, SlotIndex) || !DragPreviewClass) return;
	UWizardSkillDrag* Drag = NewObject<UWizardSkillDrag>(this);
	Drag->Request = {Definition->SkillId, SlotIndex, MenuController};
	WizardIconDrag::SetVisual(Drag, GetOwningPlayer(), DragPreviewClass, Definition->Icon);
	Operation = Drag;
}
bool UWizardSkillTile::NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event, UDragDropOperation* Operation)
{
	const UWizardSkillDrag* Drag = Cast<UWizardSkillDrag>(Operation);
	return MenuController.IsValid() && Drag && SlotIndex != INDEX_NONE && MenuController->HandleDrop(Drag->Request, SlotIndex);
}
UWizardSkillMenu::UWizardSkillMenu()
{
	bIsBackHandler = true;
	SetIsFocusable(true);
}
TOptional<FUIInputConfig> UWizardSkillMenu::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, EMouseLockMode::DoNotLock, false);
}
void UWizardSkillMenu::NativeConstruct()
{
	Super::NativeConstruct();
	if (CloseButton) CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::CloseMenu);
}
void UWizardSkillMenu::NativeOnActivated()
{
	Super::NativeOnActivated();
	Unbind();
	AWizardPlayerState* State = GetOwningPlayerState<AWizardPlayerState>();
	MenuController = NewObject<UWizardSkillMenuController>(this);
	if (!MenuController->Open(State ? State->GetSkills() : nullptr)) { DeactivateWidget(); return; }
	MenuController->OnChanged.AddUObject(this, &ThisClass::Refresh);
	MenuController->OnFeedback.AddUObject(this, &ThisClass::HandleFeedback);
	HandleFeedback(EWizardSkillResult::Success);
	Refresh();
}
void UWizardSkillMenu::Refresh()
{
	if (!Candidates || !LoadoutSlots || !TileClass || !MenuController) return;
	Candidates->ClearChildren();
	LoadoutSlots->ClearChildren();
	TArray<FWizardSkillMenuEntry> CandidateEntries, SlotEntries;
	MenuController->GetEntries(CandidateEntries, SlotEntries);
	int32 CandidateIndex = 0;
	for (const FWizardSkillMenuEntry& Entry : CandidateEntries)
	{
		UWizardSkillTile* Tile = CreateWidget<UWizardSkillTile>(GetOwningPlayer(), TileClass);
		if (!Tile) continue;
		if (UUniformGridPanel* Grid = Cast<UUniformGridPanel>(Candidates)) Grid->AddChildToUniformGrid(Tile, CandidateIndex / 6, CandidateIndex % 6);
		else Candidates->AddChild(Tile);
		++CandidateIndex;
		Tile->Configure(Entry.Definition, Entry.Slot, MenuController);
		Tile->SetStatus(TEXT(""), Entry.bAvailable);
	}
	for (const FWizardSkillMenuEntry& Entry : SlotEntries)
	{
		UWizardSkillTile* Tile = CreateWidget<UWizardSkillTile>(GetOwningPlayer(), TileClass);
		if (!Tile) continue;
		LoadoutSlots->AddChild(Tile);
		Tile->Configure(Entry.Definition, Entry.Slot, MenuController);
		Tile->SetStatus(FString::FromInt(Entry.Slot + 1), Entry.bAvailable);
	}
}
void UWizardSkillMenu::HandleFeedback(EWizardSkillResult Result) { if (Feedback) Feedback->SetText(WizardSkills::ResultText(Result)); }
bool UWizardSkillMenu::NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event, UDragDropOperation* Operation)
{
	// 只接受上方滚动区域；卡片和空白处的未处理Drop都会冒泡到此，其他页面空白不卸下。
	if (LibraryScroll && LibraryScroll->GetCachedGeometry().IsUnderLocation(Event.GetScreenSpacePosition()))
	{
		const UWizardSkillDrag* Drag = Cast<UWizardSkillDrag>(Operation);
		return MenuController && Drag && MenuController->HandleDrop(Drag->Request, INDEX_NONE);
	}
	return Super::NativeOnDrop(Geometry, Event, Operation);
}
void UWizardSkillMenu::Unbind()
{
	if (MenuController) MenuController->Close();
	MenuController = nullptr;
	UWidgetBlueprintLibrary::CancelDragDrop();
}
void UWizardSkillMenu::NativeOnDeactivated() { Unbind(); Super::NativeOnDeactivated(); }
void UWizardSkillMenu::NativeDestruct()
{
	if (CloseButton) CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::CloseMenu);
	Unbind();
	Super::NativeDestruct();
}
void UWizardSkillMenu::CloseMenu() { DeactivateWidget(); }
bool UWizardSkillMenu::NativeOnHandleBackAction() { CloseMenu(); return true; }

void UWizardSkillBar::NativeConstruct()
{
	Super::NativeConstruct();
	AWizardPlayerState* State = GetOwningPlayerState<AWizardPlayerState>();
	Skills = State ? State->GetSkills() : nullptr;
	if (Skills.IsValid()) ChangedHandle = Skills->OnLoadoutChanged.AddUObject(this, &ThisClass::RebuildSlots);
	if (AWizardPlayerController* Controller = Cast<AWizardPlayerController>(GetOwningPlayer())) FeedbackHandle = Controller->OnSkillFeedback.AddUObject(this, &ThisClass::HandleFeedback);
	RebuildSlots();
}
void UWizardSkillBar::RebuildSlots()
{
	if (!SkillSlots || !TileClass || !Skills.IsValid()) return;
	SkillSlots->ClearChildren();
	Tiles.Reset();
	const AWizardPlayerState* State = GetOwningPlayerState<AWizardPlayerState>();
	for (int32 SlotIndex = 0; SlotIndex < 4; ++SlotIndex)
	{
		UWizardSkillTile* Tile = CreateWidget<UWizardSkillTile>(GetOwningPlayer(), TileClass);
		SkillSlots->AddChild(Tile);
		Tile->Configure(Skills->GetSlot(SlotIndex), SlotIndex, nullptr);
		Tile->SetStatus(FString::FromInt(SlotIndex + 1), true);
		const UWizardSkillDefinition* Definition = Skills->GetSlot(SlotIndex);
		Tile->SetHUDState(Skills->GetCooldownRemaining(Definition), !Definition || (State && State->GetAttributes()->GetMana() >= Definition->ManaCost));
		Tiles.Add(Tile);
	}
}
void UWizardSkillBar::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	RefreshTime += DeltaSeconds;
	if (RefreshTime < .1f || !Skills.IsValid()) return;
	RefreshTime = 0.f;
	const AWizardPlayerState* State = GetOwningPlayerState<AWizardPlayerState>();
	for (int32 SlotIndex = 0; SlotIndex < Tiles.Num(); ++SlotIndex)
	{
		const UWizardSkillDefinition* Definition = Skills->GetSlot(SlotIndex);
		const float Time = Skills->GetCooldownRemaining(Definition);
		const bool bMana = !Definition || (State && State->GetAttributes()->GetMana() >= Definition->ManaCost);
		Tiles[SlotIndex]->SetStatus(FString::FromInt(SlotIndex + 1), Time <= 0.f && bMana);
		Tiles[SlotIndex]->SetHUDState(Time, bMana);
	}
}
void UWizardSkillBar::HandleFeedback(EWizardSkillResult Result) { if (Feedback) Feedback->SetText(WizardSkills::ResultText(Result)); }
void UWizardSkillBar::NativeDestruct()
{
	if (Skills.IsValid()) Skills->OnLoadoutChanged.Remove(ChangedHandle);
	if (AWizardPlayerController* Controller = Cast<AWizardPlayerController>(GetOwningPlayer())) Controller->OnSkillFeedback.Remove(FeedbackHandle);
	Skills.Reset();
	Tiles.Reset();
	Super::NativeDestruct();
}
