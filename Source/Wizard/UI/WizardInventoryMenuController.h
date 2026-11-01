// Wizard 项目：背包菜单会话持有版本请求，视图不直接修改物品状态。
#pragma once
#include "CoreMinimal.h"
#include "Inventory/WizardInventoryComponent.h"
#include "WizardInventoryMenuController.generated.h"

class UWizardInventoryMenuController;
struct FWizardInventoryDragRequest
{
	FWizardInventoryRequest Item;
	TWeakObjectPtr<UWizardInventoryMenuController> Session;
	FGuid SessionId;
};
// 打开数量弹窗时冻结来源；用户之后选择其他格，不改变这次操作的目标。
struct FWizardInventoryQuantityRequest
{
	FWizardInventoryRequest Item;
	FGuid SessionId;
	bool bDiscard = false;
};
DECLARE_MULTICAST_DELEGATE(FWizardInventoryMenuChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FWizardInventoryMenuFeedback, EWizardInventoryResult);
namespace WizardInventory { WIZARD_API FText ResultText(EWizardInventoryResult Result); }

UCLASS()
class WIZARD_API UWizardInventoryMenuController : public UObject
{
	GENERATED_BODY()
public:
	bool Open(UWizardInventoryComponent* InModel);
	void Close();
	FWizardInventorySnapshot GetSnapshot() const;
	void Select(int32 Slot, bool bEquipment = false);
	int32 GetSelectedSlot() const { return Selected.Slot; }
	bool IsEquipmentSelected() const { return Selected.bEquipment; }
	FWizardInventoryEntry GetSelectedEntry() const;
	bool MakeDragRequest(int32 Slot, FWizardInventoryDragRequest& Request, bool bEquipment = false) const;
	bool HandleDrop(const FWizardInventoryDragRequest& Request, int32 TargetSlot, bool bEquipment = false);
	void UseSelected();
	bool PrepareQuantity(bool bDiscard, FWizardInventoryQuantityRequest& Request);
	void SubmitQuantity(const FWizardInventoryQuantityRequest& Request, int32 Quantity);
	void SplitSelected(int32 Quantity);
	void DiscardSelected(int32 Quantity);
	FWizardInventoryMenuChanged OnChanged;
	FWizardInventoryMenuFeedback OnFeedback;
protected:
	virtual void BeginDestroy() override;
private:
	void HandleModelChanged();
	void Report(EWizardInventoryResult Result);
	TWeakObjectPtr<UWizardInventoryComponent> Model;
	FWizardInventoryRequest Selected;
	FDelegateHandle ChangedHandle;
	FGuid SessionId;
};
