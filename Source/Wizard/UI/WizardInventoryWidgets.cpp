#include "UI/WizardInventoryWidgets.h"
#include "UI/WizardAttributeDisplay.h"
#include "UI/WizardIconDragPreview.h"
#include "WizardPlayerState.h"
#include "AbilitySystemComponent.h"
#include "GAS/WizardAttributeSet.h"
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

namespace
{
	TArray<FGameplayAttribute> DisplayAttributes()
	{
		return { UWizardAttributeSet::GetHealthAttribute(), UWizardAttributeSet::GetMaxHealthAttribute(),
			UWizardAttributeSet::GetManaAttribute(), UWizardAttributeSet::GetMaxManaAttribute(),
			UWizardAttributeSet::GetAttackPowerAttribute(), UWizardAttributeSet::GetDefenseAttribute(),
			UWizardAttributeSet::GetMoveSpeedAttribute(), UWizardAttributeSet::GetShieldAttribute() };
	}
}

UWizardInventoryTile::UWizardInventoryTile(const FObjectInitializer& Initializer) : Super(Initializer) { SetIsFocusable(true); }
void UWizardInventoryTile::Configure(const FWizardInventoryEntry& Entry, int32 Index, UWizardInventoryMenuController* Controller, bool bEquipment, UTexture2D* EmptySlotIcon)
{
	MenuController = Controller;
	SlotIndex = Index;
	bEquipmentSlot = bEquipment;
	const UWizardItemDefinition* Definition = Entry.Definition.Get();
	const bool bPlaceholder = bEquipment && Entry.IsEmpty();
	IconTexture = Definition ? Definition->GetIconTexture() : bPlaceholder ? EmptySlotIcon : nullptr;
	if (Icon)
	{
		Icon->SetBrushFromTexture(IconTexture);
		Icon->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, bPlaceholder ? .28f : 1.f));
		Icon->SetRenderScale(bPlaceholder ? FVector2D(.62f, .62f) : FVector2D(1.f, 1.f));
		Icon->SetVisibility(IconTexture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
	if (QuantityText) QuantityText->SetText(Entry.IsEmpty() || (Definition && Definition->Type == EWizardItemType::Equipment) ? FText::GetEmpty() : FText::AsNumber(Entry.Quantity));
	if (SelectionBorder) SelectionBorder->SetBrushColor(!Entry.IsEmpty() && Controller && Controller->GetSelectedSlot() == Index && Controller->IsEquipmentSelected() == bEquipmentSlot ? FLinearColor(.25f,.55f,.85f,1.f) : FLinearColor(.07f,.09f,.14f,1.f));
}
void UWizardInventoryTile::NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event)
{
	Super::NativeOnMouseEnter(Geometry, Event);
	if (!UWidgetBlueprintLibrary::IsDragDropping()) OnInspect.Broadcast(SlotIndex, this, false);
}
FReply UWizardInventoryTile::NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
	if (Event.GetEffectingButton() != EKeys::LeftMouseButton) return Super::NativeOnMouseButtonDown(Geometry, Event);
	bCanDrag = MenuController.IsValid() && MenuController->MakeDragRequest(SlotIndex, PendingDrag, bEquipmentSlot);
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
	return MenuController.IsValid() && Drag && MenuController->HandleDrop(Drag->Request, SlotIndex, bEquipmentSlot);
}
void UWizardInventoryPopover::NativeConstruct()
{
	Super::NativeConstruct();
	if (UseButton) UseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleUse);
	if (SplitButton) SplitButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSplit);
	if (DiscardButton) DiscardButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleDiscard);
	if (SellButton) SellButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleSell);
}
void UWizardInventoryPopover::Configure(const UWizardItemDefinition* Definition, int32 Quantity, bool bEquipped)
{
	if (SellButton) SellButton->SetVisibility(ESlateVisibility::Collapsed);
	if (UseButton) UseButton->SetVisibility(ESlateVisibility::Visible);
	FString Details;
	if (Definition)
	{
		Details = Definition->DisplayName.ToString();
		if (!Definition->Description.IsEmpty()) Details += TEXT("\n") + Definition->Description.ToString();
		if (Definition->Type == EWizardItemType::Equipment)
		{
			for (const auto& Bonus : Definition->EquipmentBonuses)
			{
				Details += FString::Printf(TEXT("\n%s  %s%s"), *WizardAttributeDisplay::GetLabel(Bonus.Attribute), Bonus.Magnitude >= 0.f ? TEXT("+") : TEXT(""), *FText::AsNumber(Bonus.Magnitude).ToString());
			}
		}
		else Details += FString::Printf(TEXT("\n数量：%s"), *FText::AsNumber(Quantity).ToString());
	}
	if (DetailsText) DetailsText->SetText(FText::FromString(Details));
	if (UseLabel) UseLabel->SetText(FText::FromString(bEquipped ? TEXT("卸下") : Definition && Definition->Type == EWizardItemType::Equipment ? TEXT("装备") : TEXT("使用")));
	if (UseButton) UseButton->SetIsEnabled(Definition && (bEquipped || Definition->Type == EWizardItemType::Consumable || Definition->Type == EWizardItemType::Equipment));
	if (SplitButton) SplitButton->SetVisibility(!bEquipped && Definition && Quantity > 1 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (DiscardButton) DiscardButton->SetVisibility(!bEquipped && Definition ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}
void UWizardInventoryPopover::ConfigureTrading(const UWizardItemDefinition* Definition, int32 Quantity)
{
	Configure(Definition, Quantity);
	if (UseButton) UseButton->SetVisibility(ESlateVisibility::Collapsed);
	if (SplitButton) SplitButton->SetVisibility(ESlateVisibility::Collapsed);
	if (DiscardButton) DiscardButton->SetVisibility(ESlateVisibility::Collapsed);
	if (DetailsText && Definition) DetailsText->SetText(FText::FromString(DetailsText->GetText().ToString() + FString::Printf(TEXT("\n售价  %s"), *FText::AsNumber(Definition->SellPrice).ToString())));
	if (SellButton) { SellButton->SetVisibility(ESlateVisibility::Visible); SellButton->SetIsEnabled(Definition && Definition->bTradable && Definition->SellPrice > 0); }
}
UWidget* UWizardInventoryPopover::GetFirstAction() const
{
	if (SellButton && SellButton->IsVisible()) return SellButton;
	return UseButton && UseButton->GetIsEnabled() ? UseButton.Get() : DiscardButton.Get();
}
void UWizardInventoryPopover::HandleUse() { OnUse.Broadcast(); }
void UWizardInventoryPopover::HandleSplit() { OnSplit.Broadcast(); }
void UWizardInventoryPopover::HandleDiscard() { OnDiscard.Broadcast(); }
void UWizardInventoryPopover::HandleSell() { OnSell.Broadcast(); }
void UWizardInventoryPopover::NativeDestruct()
{
	if (UseButton) UseButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleUse);
	if (SplitButton) SplitButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleSplit);
	if (DiscardButton) DiscardButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleDiscard);
	if (SellButton) SellButton->OnClicked.RemoveDynamic(this, &ThisClass::HandleSell);
	OnSell.Clear();
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
	TransactionSubmit = {};
	MenuController = Controller; Pending = Request; MaximumQuantity = Maximum;
	if (Heading) Heading->SetText(FText::FromString(Request.bDiscard ? TEXT("丢弃") : TEXT("拆分")));
	if (QuantitySlider) { QuantitySlider->SetMinValue(0.f); QuantitySlider->SetMaxValue(1.f); QuantitySlider->SetStepSize(Maximum > 1 ? static_cast<float>(1. / (static_cast<double>(Maximum) - 1.)) : 1.f); QuantitySlider->SetIsEnabled(Maximum > 1); }
	SetQuantity(Request.bDiscard ? Maximum : 1);
	if (QuantityInput) QuantityInput->SetKeyboardFocus();
	if (Feedback) Feedback->SetText(FText::GetEmpty());
}
void UWizardInventoryQuantityModal::ConfigureTransaction(FText Title, int32 Maximum, TFunction<void(int32)> Submit)
{
	Configure(nullptr, {}, Maximum);
	TransactionSubmit = MoveTemp(Submit);
	if (Heading) Heading->SetText(Title);
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
	if (TransactionSubmit) { auto Submit = MoveTemp(TransactionSubmit); Submit(CurrentQuantity); }
	else if (MenuController.IsValid()) MenuController->SubmitQuantity(Pending, CurrentQuantity);
	DeactivateWidget();
}
void UWizardInventoryQuantityModal::Cancel() { DeactivateWidget(); }
bool UWizardInventoryQuantityModal::NativeOnHandleBackAction() { Cancel(); return true; }
void UWizardInventoryQuantityModal::NativeOnDeactivated() { TransactionSubmit = {}; MenuController.Reset(); Pending = {}; Super::NativeOnDeactivated(); }
void UWizardInventoryQuantityModal::NativeDestruct()
{
	MenuController.Reset(); Pending = {};
	TransactionSubmit = {};
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
	PlayerStateSource = State;
	if (State)
	{
		GoldHandle = State->OnGoldChanged.AddUObject(this, &ThisClass::RefreshGold);
		AttributeSource = State->GetAbilitySystemComponent();
		if (AttributeSource.IsValid())
		{
			for (const FGameplayAttribute& Attribute : DisplayAttributes())
			{
				AttributeHandles.Add(AttributeSource->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(this, &ThisClass::HandleAttributeChanged));
			}
		}
	}
	RefreshAttributes(); RefreshGold();
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
	if (EquipmentGrid)
	{
		if (EquipmentTiles.Num() != Snapshot.Equipment.Num())
		{
			EquipmentGrid->ClearChildren(); EquipmentTiles.Reset();
			for (int32 Index = 0; Index < Snapshot.Equipment.Num(); ++Index)
			{
				UWizardInventoryTile* Tile = CreateWidget<UWizardInventoryTile>(GetOwningPlayer(), TileClass);
				if (!Tile) continue;
				Tile->OnInspect.AddUObject(this, &ThisClass::Inspect);
				Tile->OnBeginDrag.AddUObject(this, &ThisClass::HideDetails);
				EquipmentGrid->AddChildToUniformGrid(Tile, Index, 0); EquipmentTiles.Add(Tile);
			}
		}
		for (int32 Index = 0; Index < EquipmentTiles.Num(); ++Index)
		{
			UTexture2D* EmptyIcon = EquipmentSlotIcons.IsValidIndex(Index) ? EquipmentSlotIcons[Index].LoadSynchronous() : nullptr;
			EquipmentTiles[Index]->Configure(Snapshot.Equipment[Index], Index, MenuController, true, EmptyIcon);
		}
	}
	if (DetailsPopover && DetailsPopover->IsVisible())
	{
		const auto Selected = MenuController->GetSelectedEntry();
		const UWizardItemDefinition* Definition = Selected.Definition.Get();
		if (!Definition) HideDetails(); else DetailsPopover->Configure(Definition, Selected.Quantity, MenuController->IsEquipmentSelected());
	}
}
void UWizardInventoryMenu::HandleAttributeChanged(const FOnAttributeChangeData& Data) { RefreshAttributes(); }
void UWizardInventoryMenu::RefreshAttributes()
{
	if (!AttributeText) return;
	if (!AttributeSource.IsValid()) { AttributeText->SetText(FText::GetEmpty()); return; }
	const UAbilitySystemComponent* Source = AttributeSource.Get();
	const auto Number = [Source](const FGameplayAttribute& Attribute) { return FText::AsNumber(Source->GetNumericAttribute(Attribute)).ToString(); };
	const FString Text = FString::Printf(TEXT("生命  %s / %s\n\n法力  %s / %s\n\n攻击力  %s\n\n防御力  %s\n\n移动速度  %s\n\n护盾  %s"),
		*Number(UWizardAttributeSet::GetHealthAttribute()), *Number(UWizardAttributeSet::GetMaxHealthAttribute()),
		*Number(UWizardAttributeSet::GetManaAttribute()), *Number(UWizardAttributeSet::GetMaxManaAttribute()),
		*Number(UWizardAttributeSet::GetAttackPowerAttribute()), *Number(UWizardAttributeSet::GetDefenseAttribute()),
		*Number(UWizardAttributeSet::GetMoveSpeedAttribute()), *Number(UWizardAttributeSet::GetShieldAttribute()));
	AttributeText->SetText(FText::FromString(Text));
}
void UWizardInventoryMenu::RefreshGold()
{
	if (GoldText) GoldText->SetText(FText::AsNumber(PlayerStateSource.IsValid() ? PlayerStateSource->GetGold() : 0));
}
void UWizardInventoryMenu::Inspect(int32 Index, UWizardInventoryTile* Source, bool bKeyboard)
{
	if (!MenuController || !DetailsPopover || UWidgetBlueprintLibrary::IsDragDropping()) return;
	MenuController->Select(Index, Source && Source->IsEquipmentSlot());
	const auto Entry = MenuController->GetSelectedEntry();
	const UWizardItemDefinition* Definition = Entry.Definition.Get();
	if (!Definition) { HideDetails(); return; }
	HoverSource = Source; OutsideTime = 0.f; bKeyboardDetails = bKeyboard;
	DetailsPopover->Configure(Definition, Entry.Quantity, MenuController->IsEquipmentSelected());
	DetailsPopover->SetVisibility(ESlateVisibility::Visible);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(DetailsPopover->Slot))
	{
		const FGeometry& SourceGeometry = Source->GetCachedGeometry();
		FVector2D Position = GetCachedGeometry().AbsoluteToLocal(SourceGeometry.LocalToAbsolute(FVector2D(SourceGeometry.GetLocalSize().X + 6.f, 0)));
		const FVector2D Available = GetCachedGeometry().GetLocalSize();
		DetailsPopover->ForceLayoutPrepass();
		const FVector2D Desired = DetailsPopover->GetDesiredSize();
		const FVector2D DetailsSize(FMath::Max(310.f, Desired.X), FMath::Max(250.f, Desired.Y));
		Position.X = FMath::Clamp(Position.X, 0.f, FMath::Max(0.f, Available.X - DetailsSize.X));
		Position.Y = FMath::Clamp(Position.Y, 0.f, FMath::Max(0.f, Available.Y - DetailsSize.Y));
		CanvasSlot->SetPosition(Position);
	}
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
	if (AttributeSource.IsValid())
	{
		const auto Attributes = DisplayAttributes();
		for (int32 Index = 0; Index < AttributeHandles.Num(); ++Index)
		{
			AttributeSource->GetGameplayAttributeValueChangeDelegate(Attributes[Index]).Remove(AttributeHandles[Index]);
		}
	}
	AttributeHandles.Reset(); AttributeSource.Reset();
	if (PlayerStateSource.IsValid()) PlayerStateSource->OnGoldChanged.Remove(GoldHandle);
	GoldHandle.Reset(); PlayerStateSource.Reset();
	HideDetails(); if (MenuController) MenuController->Close(); MenuController = nullptr; UWidgetBlueprintLibrary::CancelDragDrop();
}
void UWizardInventoryMenu::NativeOnDeactivated() { CloseSession(); Super::NativeOnDeactivated(); }
bool UWizardInventoryMenu::NativeOnHandleBackAction() { DeactivateWidget(); return true; }
void UWizardInventoryMenu::NativeDestruct()
{
	CloseSession(); if (ToggleHandle.IsValid()) { RemoveActionBinding(ToggleHandle); ToggleHandle.Unregister(); } ToggleHandle = {};
	if (DetailsPopover) { DetailsPopover->OnUse.RemoveAll(this); DetailsPopover->OnSplit.RemoveAll(this); DetailsPopover->OnDiscard.RemoveAll(this); }
	for (UWizardInventoryTile* Tile : EquipmentTiles) if (Tile) { Tile->OnInspect.RemoveAll(this); Tile->OnBeginDrag.RemoveAll(this); }
	for (UWizardInventoryTile* Tile : Tiles) if (Tile) { Tile->OnInspect.RemoveAll(this); Tile->OnBeginDrag.RemoveAll(this); }
	// 重建视图时重新创建格子及其订阅，不复用已解绑的格子。
	Tiles.Reset(); EquipmentTiles.Reset();
	Super::NativeDestruct();
}
