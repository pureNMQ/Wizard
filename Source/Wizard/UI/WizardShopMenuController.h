// Wizard 项目：商店会话冻结目录报价和背包来源，视图只提交数量。
#pragma once
#include "CoreMinimal.h"
#include "Inventory/WizardInventoryComponent.h"
#include "WizardShopMenuController.generated.h"
class UWizardShopCatalog;
struct FWizardShopQuantityRequest
{
	FGuid SessionId;
	FWizardInventoryRequest Item;
	int32 ProductIndex = INDEX_NONE;
	int32 UnitPrice = 0;
	TWeakObjectPtr<const UWizardItemDefinition> Definition;
	bool bSell = false;
};
DECLARE_MULTICAST_DELEGATE(FWizardShopChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FWizardShopFeedback, EWizardInventoryResult);
UCLASS()
class WIZARD_API UWizardShopMenuController : public UObject
{
	GENERATED_BODY()
public:
	bool Open(UWizardInventoryComponent* Inventory, const UWizardShopCatalog* Catalog, TFunction<bool()> Permission);
	void Close();
	FWizardInventorySnapshot GetSnapshot() const;
	bool PreparePurchase(int32 ProductIndex, FWizardShopQuantityRequest& Request, int32& Maximum);
	bool PrepareSale(int32 Slot, FWizardShopQuantityRequest& Request, int32& Maximum);
	void SubmitQuantity(const FWizardShopQuantityRequest& Request, int32 Quantity);
	FWizardShopChanged OnChanged;
	FWizardShopFeedback OnFeedback;
protected:
	virtual void BeginDestroy() override;
private:
	void Changed();
	bool CheckPermission();
	TWeakObjectPtr<UWizardInventoryComponent> Model;
	UPROPERTY(Transient) TObjectPtr<const UWizardShopCatalog> Catalog;
	TFunction<bool()> PermissionCheck;
	FDelegateHandle ChangedHandle;
	FGuid SessionId;
};
