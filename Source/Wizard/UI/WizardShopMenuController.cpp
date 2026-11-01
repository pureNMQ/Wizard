#include "UI/WizardShopMenuController.h"
#include "Trading/WizardShopCatalog.h"
bool UWizardShopMenuController::Open(UWizardInventoryComponent* Inventory, const UWizardShopCatalog* InCatalog, TFunction<bool()> Permission)
{
	Close();
	if (!IsValid(Inventory) || !IsValid(InCatalog) || !Permission || !Permission()) return false;
	Model = Inventory; Catalog = InCatalog; PermissionCheck = MoveTemp(Permission); SessionId = FGuid::NewGuid();
	ChangedHandle = Model->OnChanged.AddUObject(this, &ThisClass::Changed);
	return true;
}
void UWizardShopMenuController::Close()
{
	if (Model.IsValid()) Model->OnChanged.Remove(ChangedHandle);
	ChangedHandle.Reset(); Model.Reset(); Catalog = nullptr; PermissionCheck = {}; SessionId.Invalidate();
	OnChanged.Clear(); OnFeedback.Clear();
}
bool UWizardShopMenuController::CheckPermission()
{
	if (Model.IsValid() && Catalog && SessionId.IsValid() && PermissionCheck && PermissionCheck()) return true;
	OnFeedback.Broadcast(EWizardInventoryResult::ShopUnavailable); return false;
}
FWizardInventorySnapshot UWizardShopMenuController::GetSnapshot() const { return Model.IsValid() ? Model->GetSnapshot() : FWizardInventorySnapshot(); }
bool UWizardShopMenuController::PreparePurchase(int32 ProductIndex, FWizardShopQuantityRequest& Request, int32& Maximum)
{
	Request = {}; Maximum = 0;
	if (!CheckPermission()) return false;
	if (!Catalog->Products.IsValidIndex(ProductIndex)) { OnFeedback.Broadcast(EWizardInventoryResult::UnknownDefinition); return false; }
	const auto& Product = Catalog->Products[ProductIndex];
	Maximum = Model->GetPurchaseLimit(Catalog, ProductIndex);
	if (Maximum < 1)
	{
		OnFeedback.Broadcast(Model->CheckPurchase(Catalog, ProductIndex, 1, GetSnapshot().Revision));
		return false;
	}
	Request.SessionId = SessionId; Request.ProductIndex = ProductIndex;
	Request.UnitPrice = Product.BuyPrice; Request.Definition = Product.Definition.Get(); Request.Item.Revision = GetSnapshot().Revision;
	return true;
}
bool UWizardShopMenuController::PrepareSale(int32 Slot, FWizardShopQuantityRequest& Request, int32& Maximum)
{
	Request = {}; Maximum = 0;
	if (!CheckPermission()) return false;
	const auto Snapshot = GetSnapshot();
	if (!Snapshot.Slots.IsValidIndex(Slot) || Snapshot.Slots[Slot].IsEmpty()) { OnFeedback.Broadcast(EWizardInventoryResult::StaleRequest); return false; }
	const auto& Entry = Snapshot.Slots[Slot];
	if (!Entry.Definition->bTradable || Entry.Definition->SellPrice <= 0) { OnFeedback.Broadcast(EWizardInventoryResult::NotTradable); return false; }
	Request.SessionId = SessionId; Request.Item = Model->MakeRequest(Slot); Request.bSell = true;
	Request.Definition = Entry.Definition.Get(); Request.UnitPrice = Entry.Definition->SellPrice; Maximum = Entry.Quantity;
	return true;
}
void UWizardShopMenuController::SubmitQuantity(const FWizardShopQuantityRequest& Request, int32 Quantity)
{
	if (!CheckPermission()) return;
	if (Request.SessionId != SessionId || Request.Item.Revision != GetSnapshot().Revision || !Request.Definition.IsValid())
	{ OnFeedback.Broadcast(EWizardInventoryResult::StaleRequest); return; }
	if (Request.bSell)
	{
		if (Request.Definition->SellPrice != Request.UnitPrice) { OnFeedback.Broadcast(EWizardInventoryResult::StaleRequest); return; }
		OnFeedback.Broadcast(Model->Sell(Request.Item, Quantity));
	}
	else
	{
		if (!Catalog->Products.IsValidIndex(Request.ProductIndex) || Catalog->Products[Request.ProductIndex].Definition != Request.Definition.Get() || Catalog->Products[Request.ProductIndex].BuyPrice != Request.UnitPrice)
		{ OnFeedback.Broadcast(EWizardInventoryResult::StaleRequest); return; }
		OnFeedback.Broadcast(Model->Buy(Catalog, Request.ProductIndex, Quantity, Request.Item.Revision));
	}
}
void UWizardShopMenuController::Changed() { OnChanged.Broadcast(); }
void UWizardShopMenuController::BeginDestroy() { Close(); Super::BeginDestroy(); }
