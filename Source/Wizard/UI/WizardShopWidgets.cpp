#include "UI/WizardShopWidgets.h"
#include "UI/WizardAttributeDisplay.h"
#include "UI/WizardInventoryWidgets.h"
#include "UI/WizardUIRoot.h"
#include "Trading/WizardShopActor.h"
#include "Trading/WizardShopCatalog.h"
#include "WizardPlayerState.h"
#include "WizardPlayerController.h"
#include "Components/VerticalBox.h"
#include "Components/UniformGridPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Framework/Application/SlateApplication.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

void UWizardShopProductRow::NativeConstruct()
{
	Super::NativeConstruct(); if (SelectButton) SelectButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Select);
}
void UWizardShopProductRow::Configure(UWizardShopMenu* Menu, int32 ProductIndex, const UWizardItemDefinition* Definition, int32 Price)
{
	OwnerMenu = Menu; Index = ProductIndex;
	if (NameText) NameText->SetText(Definition ? Definition->DisplayName : FText::GetEmpty());
	if (PriceText) PriceText->SetText(FText::AsNumber(Price));
	if (Icon) { UTexture2D* Texture = Definition ? Definition->GetIconTexture() : nullptr; Icon->SetBrushFromTexture(Texture); Icon->SetVisibility(Texture ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden); }
}
void UWizardShopProductRow::Select() { if (OwnerMenu.IsValid()) OwnerMenu->SelectProduct(Index); }
void UWizardShopProductRow::SetSelected(int32 ProductIndex)
{
	if (SelectionBorder) SelectionBorder->SetBrushColor(ProductIndex == Index ? FLinearColor(.20f,.25f,.21f,1.f) : FLinearColor(.07f,.09f,.11f,1.f));
}
void UWizardShopProductRow::FocusSelection()
{
	if (SelectButton && SelectButton->GetIsEnabled()) SelectButton->SetUserFocus(GetOwningPlayer());
}
void UWizardShopProductRow::NativeDestruct()
{
	if (SelectButton) SelectButton->OnClicked.RemoveDynamic(this, &ThisClass::Select);
	OwnerMenu.Reset(); Super::NativeDestruct();
}
UWizardShopMenu::UWizardShopMenu() { bIsBackHandler = true; SetIsFocusable(true); }
TOptional<FUIInputConfig> UWizardShopMenu::GetDesiredInputConfig() const { return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture, EMouseLockMode::DoNotLock, false); }
UWidget* UWizardShopMenu::NativeGetDesiredFocusTarget() const { return CloseButton; }
void UWizardShopMenu::NativeConstruct()
{
	Super::NativeConstruct();
	if (BuyButton) BuyButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Buy);
	if (CloseButton) CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::CloseMenu);
	if (AllButton) AllButton->OnClicked.AddUniqueDynamic(this, &ThisClass::ShowAll);
	if (EquipmentButton) EquipmentButton->OnClicked.AddUniqueDynamic(this, &ThisClass::ShowEquipment);
	if (ConsumablesButton) ConsumablesButton->OnClicked.AddUniqueDynamic(this, &ThisClass::ShowConsumables);
	if (DetailsPopover) DetailsPopover->OnSell.AddUObject(this, &ThisClass::Sell);
	HideDetails();
}
void UWizardShopMenu::Configure(AWizardShopActor* Shop)
{
	CloseSession(); ShopSource = Shop; StateSource = GetOwningPlayerState<AWizardPlayerState>();
	MenuController = NewObject<UWizardShopMenuController>(this);
	TWeakObjectPtr<AWizardShopActor> WeakShop = ShopSource;
	TWeakObjectPtr<AWizardPlayerState> WeakState = StateSource;
	TWeakObjectPtr<UWizardShopCatalog> SessionCatalog = Shop ? Shop->GetCatalog() : nullptr;
	if (!MenuController->Open(StateSource.IsValid() ? StateSource->GetInventory() : nullptr, Shop ? Shop->GetCatalog() : nullptr,
		[WeakShop, WeakState, SessionCatalog]() { return WeakShop.IsValid() && WeakState.IsValid() && SessionCatalog.IsValid() && WeakShop->GetCatalog() == SessionCatalog.Get() && WeakShop->CanTrade(WeakState.Get()); })) { CloseMenu(); return; }
	MenuController->OnChanged.AddUObject(this, &ThisClass::Refresh);
	MenuController->OnFeedback.AddUObject(this, &ThisClass::ShowFeedback);
	GoldHandle = StateSource->OnGoldChanged.AddUObject(this, &ThisClass::RefreshGold);
	Category = 0; ProductIndex = INDEX_NONE; RefreshProducts(); Refresh(); RefreshGold(); ShowFeedback(EWizardInventoryResult::Success);
}
void UWizardShopMenu::RefreshProducts()
{
	if (!ProductsBox || !ProductRowClass || !ShopSource.IsValid() || !ShopSource->GetCatalog()) return;
	ProductsBox->ClearChildren();
	if (AllButton) AllButton->SetIsEnabled(Category != 0);
	if (EquipmentButton) EquipmentButton->SetIsEnabled(Category != 1);
	if (ConsumablesButton) ConsumablesButton->SetIsEnabled(Category != 2);
	const auto* Catalog = ShopSource->GetCatalog();
	bool bSelectedVisible = false;
	int32 FirstVisible = INDEX_NONE;
	for (int32 Index = 0; Index < Catalog->Products.Num(); ++Index)
	{
		const auto& Product = Catalog->Products[Index];
		if (!Product.Definition || (Category == 1 && Product.Definition->Type != EWizardItemType::Equipment) || (Category == 2 && Product.Definition->Type != EWizardItemType::Consumable)) continue;
		if (FirstVisible == INDEX_NONE) FirstVisible = Index;
		bSelectedVisible |= ProductIndex == Index;
		if (auto* Row = CreateWidget<UWizardShopProductRow>(GetOwningPlayer(), ProductRowClass)) { ProductsBox->AddChild(Row); Row->Configure(this, Index, Product.Definition, Product.BuyPrice); }
	}
	SelectProduct(bSelectedVisible ? ProductIndex : FirstVisible);
}
void UWizardShopMenu::SelectProduct(int32 Index)
{
	ProductIndex = Index;
	if (ProductsBox) for (UWidget* Child : ProductsBox->GetAllChildren()) if (auto* Row = Cast<UWizardShopProductRow>(Child)) Row->SetSelected(Index);
	const auto* Catalog = ShopSource.IsValid() ? ShopSource->GetCatalog() : nullptr;
	const bool bValid = Catalog && Catalog->Products.IsValidIndex(Index) && Catalog->Products[Index].Definition;
	if (BuyButton) BuyButton->SetIsEnabled(bValid);
	FString Text;
	if (bValid)
	{
		const auto& Product = Catalog->Products[Index];
		Text = Product.Definition->DisplayName.ToString();
		if (!Product.Definition->Description.IsEmpty()) Text += TEXT("\n") + Product.Definition->Description.ToString();
		for (const auto& Bonus : Product.Definition->EquipmentBonuses)
		{
			const FString Label = WizardAttributeDisplay::GetLabel(Bonus.Attribute);
			if (!Label.IsEmpty()) Text += FString::Printf(TEXT("\n%s %s%s"), *Label, Bonus.Magnitude >= 0 ? TEXT("+") : TEXT(""), *FText::AsNumber(Bonus.Magnitude).ToString());
		}
		Text += FString::Printf(TEXT("\n单价  %s"), *FText::AsNumber(Product.BuyPrice).ToString());
	}
	if (ProductDetails) ProductDetails->SetText(FText::FromString(Text));
}
void UWizardShopMenu::FocusProducts()
{
	// 分类切换会销毁旧商品行，也会禁用当前分类按钮；显式移交焦点给新行。
	if (ProductsBox)
	{
		for (UWidget* Child : ProductsBox->GetAllChildren())
		{
			if (auto* Row = Cast<UWizardShopProductRow>(Child)) { Row->FocusSelection(); return; }
		}
	}
	if (CloseButton) CloseButton->SetUserFocus(GetOwningPlayer());
}
void UWizardShopMenu::ShowAll() { Category = 0; RefreshProducts(); FocusProducts(); }
void UWizardShopMenu::ShowEquipment() { Category = 1; RefreshProducts(); FocusProducts(); }
void UWizardShopMenu::ShowConsumables() { Category = 2; RefreshProducts(); FocusProducts(); }
void UWizardShopMenu::Refresh()
{
	if (!MenuController || !InventoryGrid || !TileClass) return;
	const auto Snapshot = MenuController->GetSnapshot();
	if (Tiles.Num() != Snapshot.Slots.Num())
	{
		InventoryGrid->ClearChildren(); Tiles.Reset();
		for (int32 Index = 0; Index < Snapshot.Slots.Num(); ++Index)
		{
			if (auto* Tile = CreateWidget<UWizardInventoryTile>(GetOwningPlayer(), TileClass)) { Tile->OnInspect.AddUObject(this, &ThisClass::Inspect); InventoryGrid->AddChildToUniformGrid(Tile, Index / 5, Index % 5); Tiles.Add(Tile); }
		}
	}
	for (int32 Index = 0; Index < Tiles.Num(); ++Index) Tiles[Index]->Configure(Snapshot.Slots[Index], Index, nullptr);
	if (SelectedItem.Revision != Snapshot.Revision) HideDetails();
	RefreshGold();
}
void UWizardShopMenu::RefreshGold() { if (GoldText) GoldText->SetText(FText::AsNumber(StateSource.IsValid() ? StateSource->GetGold() : 0)); }
void UWizardShopMenu::ShowFeedback(EWizardInventoryResult Result)
{
	if (Feedback) Feedback->SetText(WizardInventory::ResultText(Result));
}
void UWizardShopMenu::Buy()
{
	FWizardShopQuantityRequest Request; int32 Maximum;
	if (MenuController && MenuController->PreparePurchase(ProductIndex, Request, Maximum)) OpenQuantity(Request, Maximum);
}
void UWizardShopMenu::Sell()
{
	if (!StateSource.IsValid() || !MenuController || SelectedItem.Revision != MenuController->GetSnapshot().Revision) { ShowFeedback(EWizardInventoryResult::StaleRequest); return; }
	FWizardShopQuantityRequest Request; int32 Maximum;
	if (MenuController->PrepareSale(SelectedItem.Slot, Request, Maximum)) OpenQuantity(Request, Maximum);
}
void UWizardShopMenu::OpenQuantity(const FWizardShopQuantityRequest& Request, int32 Maximum)
{
	auto* Player = Cast<AWizardPlayerController>(GetOwningPlayer());
	if (!Player || !Player->GetUIRoot() || !QuantityModalClass) return;
	HideDetails();
	if (auto* Modal = Cast<UWizardInventoryQuantityModal>(Player->GetUIRoot()->PushModal(QuantityModalClass)))
	{
		TWeakObjectPtr<UWizardShopMenuController> WeakController = MenuController;
		Modal->ConfigureTransaction(FText::FromString(Request.bSell ? TEXT("出售") : TEXT("购买")), Maximum,
			[WeakController, Request](int32 Quantity) { if (WeakController.IsValid()) WeakController->SubmitQuantity(Request, Quantity); });
	}
}
void UWizardShopMenu::Inspect(int32 Index, UWizardInventoryTile* Source, bool bKeyboard)
{
	if (!MenuController || !DetailsPopover || !StateSource.IsValid()) return;
	const auto Snapshot = MenuController->GetSnapshot();
	if (!Snapshot.Slots.IsValidIndex(Index) || Snapshot.Slots[Index].IsEmpty()) { HideDetails(); return; }
	SelectedItem = StateSource->GetInventory()->MakeRequest(Index); HoverSource = Source; OutsideTime = 0; bKeyboardDetails = bKeyboard;
	const auto& Entry = Snapshot.Slots[Index]; DetailsPopover->ConfigureTrading(Entry.Definition, Entry.Quantity); DetailsPopover->SetVisibility(ESlateVisibility::Visible);
	if (Source)
	{
		if (auto* CanvasSlot = Cast<UCanvasPanelSlot>(DetailsPopover->Slot))
		{
			DetailsPopover->ForceLayoutPrepass(); const auto Size = DetailsPopover->GetDesiredSize();
			const auto& SourceGeometry = Source->GetCachedGeometry();
			auto Position = GetCachedGeometry().AbsoluteToLocal(SourceGeometry.LocalToAbsolute(FVector2D(SourceGeometry.GetLocalSize().X + 6, 0)));
			const auto Available = GetCachedGeometry().GetLocalSize();
			Position.X = FMath::Clamp(Position.X, 0., FMath::Max(0., Available.X - Size.X));
			Position.Y = FMath::Clamp(Position.Y, 0., FMath::Max(0., Available.Y - Size.Y)); CanvasSlot->SetPosition(Position);
		}
	}
	if (bKeyboard && DetailsPopover->GetFirstAction()) DetailsPopover->GetFirstAction()->SetUserFocus(GetOwningPlayer());
}
void UWizardShopMenu::HideDetails() { if (DetailsPopover) DetailsPopover->SetVisibility(ESlateVisibility::Collapsed); HoverSource.Reset(); SelectedItem = {}; OutsideTime = 0; bKeyboardDetails = false; }
void UWizardShopMenu::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	if (!DetailsPopover || !DetailsPopover->IsVisible() || bKeyboardDetails) return;
	if (!HoverSource.IsValid()) { HideDetails(); return; }
	const auto Mouse = FSlateApplication::Get().GetCursorPos();
	const bool bInside = HoverSource->GetCachedGeometry().IsUnderLocation(Mouse) || DetailsPopover->GetCachedGeometry().IsUnderLocation(Mouse);
	OutsideTime = bInside ? 0.f : OutsideTime + DeltaSeconds; if (OutsideTime > .25f) HideDetails();
}
void UWizardShopMenu::CloseSession()
{
	if (MenuController) MenuController->Close(); MenuController = nullptr;
	if (StateSource.IsValid()) StateSource->OnGoldChanged.Remove(GoldHandle);
	GoldHandle.Reset(); StateSource.Reset(); ShopSource.Reset(); HideDetails();
}
void UWizardShopMenu::CloseMenu() { DeactivateWidget(); }
bool UWizardShopMenu::NativeOnHandleBackAction() { CloseMenu(); return true; }
void UWizardShopMenu::NativeOnDeactivated() { CloseSession(); Super::NativeOnDeactivated(); }
void UWizardShopMenu::NativeDestruct()
{
	CloseSession();
	if (BuyButton) BuyButton->OnClicked.RemoveDynamic(this, &ThisClass::Buy);
	if (CloseButton) CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::CloseMenu);
	if (AllButton) AllButton->OnClicked.RemoveDynamic(this, &ThisClass::ShowAll);
	if (EquipmentButton) EquipmentButton->OnClicked.RemoveDynamic(this, &ThisClass::ShowEquipment);
	if (ConsumablesButton) ConsumablesButton->OnClicked.RemoveDynamic(this, &ThisClass::ShowConsumables);
	if (DetailsPopover) DetailsPopover->OnSell.RemoveAll(this);
	for (UWizardInventoryTile* Tile : Tiles) if (Tile) Tile->OnInspect.RemoveAll(this);
	Tiles.Reset(); Super::NativeDestruct();
}
