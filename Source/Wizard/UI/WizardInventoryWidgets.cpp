#include "UI/WizardInventoryWidgets.h"
#include "UI/WizardIconDragPreview.h"
#include "WizardPlayerState.h"
#include "WizardPlayerController.h"
#include "UI/WizardUIRoot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/Slider.h"
#include "Components/UniformGridPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/CommonUIInputTypes.h"
#include "InputAction.h"

UWizardInventoryTile::UWizardInventoryTile(const FObjectInitializer& Initializer) : Super(Initializer) { SetIsFocusable(true); }
void UWizardInventoryTile::Configure(const FWizardInventoryEntry& Entry, int32 Index, UWizardInventoryMenuController* Controller)
{
	MenuController = Controller;
	SlotIndex = Index;
	const UWizardItemDefinition* Definition = Entry.Definition.Get();
	IconTexture = Definition ? Definition->GetIconTexture() : nullptr;
	if (Icon)
	{
		Icon->SetBrushFromTexture(IconTexture);
		Icon->SetVisibility(IconTexture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (QuantityText) QuantityText->SetText(Entry.IsEmpty() ? FText::GetEmpty() : FText::AsNumber(Entry.Quantity));
	if (SelectionBorder) SelectionBorder->SetBrushColor(Controller && Controller->GetSelectedSlot() == Index ? FLinearColor(.25f,.55f,.85f,1.f) : FLinearColor(.07f,.09f,.14f,1.f));
}
void UWizardInventoryTile::NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event)
{
	Super::NativeOnMouseEnter(Geometry, Event);
	if (!UWidgetBlueprintLibrary::IsDragDropping()) OnInspect.Broadcast(SlotIndex, this, false);
}
FReply UWizardInventoryTile::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (!MenuController.IsValid() || Event.GetEffectingButton() != EKeys::LeftMouseButton) return Super::NativeOnMouseButtonDown(Geometry, Event);
	bCanDrag = MenuController->MakeDragRequest(SlotIndex, PendingDrag);
	OnInspect.Broadcast(SlotIndex, this, false);
	return bCanDrag ? UWidgetBlueprintLibrary::DetectDragIfPressed(Event, this, EKeys::LeftMouseButton).NativeReply : FReply::Handled();
}
FReply UWizardInventoryTile::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (Event.GetKey() == EKeys::Enter || Event.GetKey() == EKeys::SpaceBar)
	{
		OnInspect.Broadcast(SlotIndex, this, true);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(Geometry, Event);
}
void UWizardInventoryTile::NativeOnDragDetected(const FGeometry& Geometry, const FPointerEvent& Event, UDragDropOperation*& Operation)
{
	if (!MenuController.IsValid() || !bCanDrag || !DragPreviewClass) return;
	OnBeginDrag.Broadcast();
	UWizardInventoryDrag* Drag = NewObject<UWizardInventoryDrag>(this);
	Drag->Request = PendingDrag;
	WizardIconDrag::SetVisual(Drag, GetOwningPlayer(), DragPreviewClass, IconTexture);
	Operation = Drag;
	bCanDrag = false;
}
bool UWizardInventoryTile::NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event, UDragDropOperation* Operation)
{
	const UWizardInventoryDrag* Drag = Cast<UWizardInventoryDrag>(Operation);
	return MenuController.IsValid() && Drag && MenuController->HandleDrop(Drag->Request, SlotIndex);
}
void UWizardInventoryPopover::NativeConstruct()
{
	Super::NativeConstruct();
	if (UseButton) UseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleUse);
	if (SplitButton) SplitButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSplit);
	if (DiscardButton) DiscardButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleDiscard);
}
void UWizardInventoryPopover::Configure(const UWizardItemDefinition* Definition, int32 Quantity)
{
	if (DetailsText) DetailsText->SetText(Definition ? FText::Format(FText::FromString(TEXT("{0}\n{1}\n数量：{2}")), Definition->DisplayName, Definition->Description, FText::AsNumber(Quantity)) : FText::GetEmpty());
	if (UseButton) UseButton->SetIsEnabled(Definition && Definition->Type == EWizardItemType::Consumable);
	if (SplitButton) SplitButton->SetIsEnabled(Definition && Quantity > 1);
	if (DiscardButton) DiscardButton->SetIsEnabled(Definition != nullptr);
}
UWidget* UWizardInventoryPopover::GetFirstAction() const { return UseButton && UseButton->GetIsEnabled() ? UseButton.Get() : DiscardButton.Get(); }
void UWizardInventoryPopover::HandleUse() { OnUse.Broadcast(); }
void UWizardInventoryPopover::HandleSplit() { OnSplit.Broadcast(); }
void UWizardInventoryPopover::HandleDiscard() { OnDiscard.Broadcast(); }
void UWizardInventoryPopover::NativeDestruct()
{
	if (UseButton) UseButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleUse);
	if (SplitButton) SplitButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleSplit);
	if (DiscardButton) DiscardButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleDiscard);
	OnUse.Clear(); OnSplit.Clear(); OnDiscard.Clear();
	Super::NativeDestruct();
}
UWizardInventoryQuantityModal::UWizardInventoryQuantityModal() { bIsBackHandler = true; bIsModal = true; SetIsFocusable(true); }
TOptional<FUIInputConfig> UWizardInventoryQuantityModal::GetDesiredInputConfig() const { return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, EMouseLockMode::DoNotLock, false); }
UWidget* UWizardInventoryQuantityModal::NativeGetDesiredFocusTarget() const { return QuantityInput; }
void UWizardInventoryQuantityModal::NativeConstruct()
{
	Super::NativeConstruct();
	if (ConfirmButton) ConfirmButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Confirm);
	if (CancelButton) CancelButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Cancel);
	if (QuantityInput)
	{
		QuantityInput->OnTextChanged.AddUniqueDynamic(this, &ThisClass::HandleQuantityTextChanged);
		QuantityInput->OnTextCommitted.AddUniqueDynamic(this, &ThisClass::HandleQuantityTextCommitted);
	}
	if (QuantitySlider) QuantitySlider->OnValueChanged.AddUniqueDynamic(this, &ThisClass::HandleSliderChanged);
}
void UWizardInventoryQuantityModal::Configure(UWizardInventoryMenuController* Controller, const FWizardInventoryQuantityRequest& Request, int32 Maximum)
{
	MenuController = Controller; Pending = Request; MaximumQuantity = Maximum;
	if (Heading) Heading->SetText(FText::FromString(Request.bDiscard ? TEXT("丢弃") : TEXT("拆分")));
	if (QuantitySlider) { QuantitySlider->SetMinValue(0.f); QuantitySlider->SetMaxValue(1.f); QuantitySlider->SetStepSize(Maximum > 1 ? static_cast<float>(1. / (static_cast<double>(Maximum) - 1.)) : 1.f); QuantitySlider->SetIsEnabled(Maximum > 1); }
	SetQuantity(Request.bDiscard ? Maximum : 1);
	if (QuantityInput) QuantityInput->SetKeyboardFocus();
	if (Feedback) Feedback->SetText(FText::GetEmpty());
}
void UWizardInventoryQuantityModal::SetQuantity(int32 Value)
{
	if (MaximumQuantity < 1) return;
	TGuardValue<bool> Guard(bSynchronizingQuantity, true);
	CurrentQuantity = FMath::Clamp(Value, 1, MaximumQuantity);
	if (QuantityInput) QuantityInput->SetText(FText::AsNumber(CurrentQuantity, &FNumberFormattingOptions::DefaultNoGrouping()));
	// Slider只是当前精确整数的近似显示。SetValue不反向驱动文本，避免大整数精度损失。
	if (QuantitySlider) QuantitySlider->SetValue(MaximumQuantity > 1 ? static_cast<float>((static_cast<double>(CurrentQuantity) - 1.) / (static_cast<double>(MaximumQuantity) - 1.)) : 0.f);
}
void UWizardInventoryQuantityModal::HandleQuantityTextChanged(const FText& Text)
{
	if (bSynchronizingQuantity || MaximumQuantity < 1) return;
	const FString Input = Text.ToString();
	if (Input.IsEmpty()) return; // 编辑暂态在失焦/确认时恢复。
	int64 Parsed = 0;
	bool bNegative = Input[0] == TCHAR('-');
	const int32 Start = bNegative ? 1 : 0;
	if (Start == Input.Len()) return;
	for (int32 Index = Start; Index < Input.Len(); ++Index)
	{
		const TCHAR Digit = Input[Index];
		if (Digit < TCHAR('0') || Digit > TCHAR('9')) return;
		Parsed = FMath::Min<int64>(MAX_int32, Parsed * 10 + Digit - TCHAR('0'));
	}
	SetQuantity(bNegative ? 1 : static_cast<int32>(Parsed));
}
void UWizardInventoryQuantityModal::HandleQuantityTextCommitted(const FText& Text, ETextCommit::Type Method)
{
	HandleQuantityTextChanged(Text);
	SetQuantity(CurrentQuantity);
}
void UWizardInventoryQuantityModal::HandleSliderChanged(float Value)
{
	if (bSynchronizingQuantity || MaximumQuantity < 1 || !FMath::IsFinite(Value)) return;
	const double Position = FMath::Clamp(static_cast<double>(Value), 0., 1.);
	const int64 Rounded = FMath::RoundToInt64(Position * (static_cast<double>(MaximumQuantity) - 1.)) + 1;
	SetQuantity(static_cast<int32>(FMath::Clamp<int64>(Rounded, 1, MaximumQuantity)));
}
void UWizardInventoryQuantityModal::Confirm()
{
	HandleQuantityTextCommitted(QuantityInput ? QuantityInput->GetText() : FText::GetEmpty(), ETextCommit::OnEnter);
	if (MaximumQuantity < 1) return;
	if (MenuController.IsValid()) MenuController->SubmitQuantity(Pending, CurrentQuantity);
	DeactivateWidget();
}
void UWizardInventoryQuantityModal::Cancel() { DeactivateWidget(); }
bool UWizardInventoryQuantityModal::NativeOnHandleBackAction() { Cancel(); return true; }
void UWizardInventoryQuantityModal::NativeOnDeactivated() { MenuController.Reset(); Pending = {}; Super::NativeOnDeactivated(); }
void UWizardInventoryQuantityModal::NativeDestruct()
{
	MenuController.Reset(); Pending = {};
	if (ConfirmButton) ConfirmButton->OnClicked.RemoveDynamic(this, &ThisClass::Confirm);
	if (CancelButton) CancelButton->OnClicked.RemoveDynamic(this, &ThisClass::Cancel);
	if (QuantityInput)
	{
		QuantityInput->OnTextChanged.RemoveDynamic(this, &ThisClass::HandleQuantityTextChanged);
		QuantityInput->OnTextCommitted.RemoveDynamic(this, &ThisClass::HandleQuantityTextCommitted);
	}
	if (QuantitySlider) QuantitySlider->OnValueChanged.RemoveDynamic(this, &ThisClass::HandleSliderChanged);
	Super::NativeDestruct();
}
UWizardInventoryMenu::UWizardInventoryMenu() { bIsBackHandler = true; SetIsFocusable(true); }
TOptional<FUIInputConfig> UWizardInventoryMenu::GetDesiredInputConfig() const { return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, EMouseLockMode::DoNotLock, false); }
void UWizardInventoryMenu::NativeConstruct()
{
	Super::NativeConstruct();
	if (DetailsPopover)
	{
		DetailsPopover->OnUse.AddUObject(this, &ThisClass::HandleUse);
		DetailsPopover->OnSplit.AddUObject(this, &ThisClass::HandleSplit);
		DetailsPopover->OnDiscard.AddUObject(this, &ThisClass::HandleDiscard);
		HideDetails();
	}
	if (InventoryAction && !ToggleHandle.IsValid()) ToggleHandle = RegisterUIActionBinding(FBindUIActionArgs(InventoryAction, false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleToggle)));
}
void UWizardInventoryMenu::NativeOnActivated()
{
	Super::NativeOnActivated(); CloseSession();
	AWizardPlayerState* State = GetOwningPlayerState<AWizardPlayerState>();
	MenuController = NewObject<UWizardInventoryMenuController>(this);
	if (!MenuController->Open(State ? State->GetInventory() : nullptr)) { DeactivateWidget(); return; }
	MenuController->OnChanged.AddUObject(this, &ThisClass::Refresh);
	MenuController->OnFeedback.AddUObject(this, &ThisClass::HandleFeedback);
	HandleFeedback(EWizardInventoryResult::Success); Refresh();
}
void UWizardInventoryMenu::Refresh()
{
	if (!MenuController || !InventoryGrid || !TileClass) return;
	const auto Snapshot = MenuController->GetSnapshot();
	if (Tiles.Num() != Snapshot.Slots.Num())
	{
		InventoryGrid->ClearChildren(); Tiles.Reset();
		for (int32 Index = 0; Index < Snapshot.Slots.Num(); ++Index)
		{
			UWizardInventoryTile* Tile = CreateWidget<UWizardInventoryTile>(GetOwningPlayer(), TileClass);
			if (!Tile) continue;
			Tile->OnInspect.AddUObject(this, &ThisClass::Inspect);
			Tile->OnBeginDrag.AddUObject(this, &ThisClass::HideDetails);
			InventoryGrid->AddChildToUniformGrid(Tile, Index / 5, Index % 5); Tiles.Add(Tile);
		}
	}
	for (int32 Index = 0; Index < Tiles.Num(); ++Index) Tiles[Index]->Configure(Snapshot.Slots[Index], Index, MenuController);
	if (DetailsPopover && DetailsPopover->IsVisible())
	{
		const auto Selected = MenuController->GetSelectedEntry();
		const UWizardItemDefinition* Definition = Selected.Definition.Get();
		if (!Definition) HideDetails(); else DetailsPopover->Configure(Definition, Selected.Quantity);
	}
}
void UWizardInventoryMenu::Inspect(int32 Index, UWizardInventoryTile* Source, bool bKeyboard)
{
	if (!MenuController || !DetailsPopover || UWidgetBlueprintLibrary::IsDragDropping()) return;
	MenuController->Select(Index);
	const auto Entry = MenuController->GetSelectedEntry();
	const UWizardItemDefinition* Definition = Entry.Definition.Get();
	if (!Definition) { HideDetails(); return; }
	HoverSource = Source; OutsideTime = 0.f; bKeyboardDetails = bKeyboard;
	DetailsPopover->Configure(Definition, Entry.Quantity);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(DetailsPopover->Slot))
	{
		const FGeometry& SourceGeometry = Source->GetCachedGeometry();
		FVector2D Position = GetCachedGeometry().AbsoluteToLocal(SourceGeometry.LocalToAbsolute(FVector2D(SourceGeometry.GetLocalSize().X + 6.f, 0)));
		const FVector2D Available = GetCachedGeometry().GetLocalSize();
		Position.X = FMath::Clamp(Position.X, 0.f, FMath::Max(0.f, Available.X - 310.f));
		Position.Y = FMath::Clamp(Position.Y, 0.f, FMath::Max(0.f, Available.Y - 250.f));
		CanvasSlot->SetPosition(Position);
	}
	DetailsPopover->SetVisibility(ESlateVisibility::Visible);
	if (bKeyboard && DetailsPopover->GetFirstAction()) DetailsPopover->GetFirstAction()->SetUserFocus(GetOwningPlayer());
}
void UWizardInventoryMenu::HideDetails()
{
	if (DetailsPopover) DetailsPopover->SetVisibility(ESlateVisibility::Collapsed);
	HoverSource.Reset(); OutsideTime = 0.f; bKeyboardDetails = false;
}
void UWizardInventoryMenu::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	if (!DetailsPopover || !DetailsPopover->IsVisible() || bKeyboardDetails) return;
	if (!HoverSource.IsValid() || !GetIsEnabled() || UWidgetBlueprintLibrary::IsDragDropping()) { HideDetails(); return; }
	const FVector2D MousePosition = FSlateApplication::Get().GetCursorPos();
	const bool bInside = HoverSource->GetCachedGeometry().IsUnderLocation(MousePosition) || DetailsPopover->GetCachedGeometry().IsUnderLocation(MousePosition);
	OutsideTime = bInside ? 0.f : OutsideTime + DeltaSeconds;
	// Slate时间在世界暂停时仍推进；短宽限允许越过格子和悬浮窗的间隙。
	if (OutsideTime > .25f) HideDetails();
}
void UWizardInventoryMenu::HandleFeedback(EWizardInventoryResult Result) { if (Feedback) Feedback->SetText(WizardInventory::ResultText(Result)); }
void UWizardInventoryMenu::HandleUse() { if (MenuController) MenuController->UseSelected(); }
void UWizardInventoryMenu::HandleSplit() { OpenQuantity(false); }
void UWizardInventoryMenu::HandleDiscard() { OpenQuantity(true); }
void UWizardInventoryMenu::OpenQuantity(bool bDiscard)
{
	if (!MenuController || !QuantityModalClass) return;
	FWizardInventoryQuantityRequest Request;
	if (!MenuController->PrepareQuantity(bDiscard, Request)) return;
	const int32 Maximum = MenuController->GetSelectedEntry().Quantity - (bDiscard ? 0 : 1);
	AWizardPlayerController* Controller = Cast<AWizardPlayerController>(GetOwningPlayer());
	if (!Controller || !Controller->GetUIRoot()) return;
	HideDetails(); UWidgetBlueprintLibrary::CancelDragDrop();
	if (UWizardInventoryQuantityModal* Modal = Cast<UWizardInventoryQuantityModal>(Controller->GetUIRoot()->PushModal(QuantityModalClass))) Modal->Configure(MenuController, Request, Maximum);
}
void UWizardInventoryMenu::HandleToggle() { if (IsActivated()) DeactivateWidget(); }
void UWizardInventoryMenu::CloseSession()
{
	HideDetails(); if (MenuController) MenuController->Close(); MenuController = nullptr; UWidgetBlueprintLibrary::CancelDragDrop();
}
void UWizardInventoryMenu::NativeOnDeactivated() { CloseSession(); Super::NativeOnDeactivated(); }
bool UWizardInventoryMenu::NativeOnHandleBackAction() { DeactivateWidget(); return true; }
void UWizardInventoryMenu::NativeDestruct()
{
	CloseSession(); if (ToggleHandle.IsValid()) { RemoveActionBinding(ToggleHandle); ToggleHandle.Unregister(); } ToggleHandle = {};
	if (DetailsPopover) { DetailsPopover->OnUse.RemoveAll(this); DetailsPopover->OnSplit.RemoveAll(this); DetailsPopover->OnDiscard.RemoveAll(this); }
	for (UWizardInventoryTile* Tile : Tiles) if (Tile) { Tile->OnInspect.RemoveAll(this); Tile->OnBeginDrag.RemoveAll(this); }
	Super::NativeDestruct();
}
