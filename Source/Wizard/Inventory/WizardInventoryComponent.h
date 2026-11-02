// Wizard 项目：所有背包写入经完整校验后一次提交，界面只持有带版本的请求。
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/WizardItemDefinition.h"
#include "WizardInventoryComponent.generated.h"
class UWizardShopCatalog;

UENUM(BlueprintType)
enum class EWizardInventoryResult : uint8
{
	Success, InvalidSlot, InvalidQuantity, UnknownDefinition, InvalidDefinition, StaleRequest,
	NoCapacity, NotConsumable, FullResource, Dead, EffectFailed, Busy, DropUnavailable, InvalidSnapshot,
	NotEquipment, WrongEquipmentSlot, NotTradable, InvalidPrice, InsufficientGold, GoldOverflow, ShopUnavailable
};

USTRUCT(BlueprintType)
struct FWizardInventoryEntry
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame) FGuid InstanceId;
	UPROPERTY(BlueprintReadOnly, SaveGame) FName DefinitionId;
	// 运行时直接持有定义；落盘仅保存稳定ID，恢复时通过AssetManager解析。
	UPROPERTY(Transient, BlueprintReadOnly) TObjectPtr<const UWizardItemDefinition> Definition;
	UPROPERTY(BlueprintReadOnly, SaveGame) int32 Quantity = 0;
	bool IsEmpty() const { return Quantity == 0 && !InstanceId.IsValid() && DefinitionId.IsNone() && !Definition; }
	bool operator==(const FWizardInventoryEntry& Other) const { return InstanceId == Other.InstanceId && DefinitionId == Other.DefinitionId && Definition == Other.Definition && Quantity == Other.Quantity; }
};

USTRUCT(BlueprintType)
struct FWizardInventorySnapshot
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame) TArray<FWizardInventoryEntry> Slots;
	UPROPERTY(BlueprintReadOnly, SaveGame) TArray<FWizardInventoryEntry> Equipment;
	// 进程内冲突检测令牌；恢复时分配新版本，不信任存档的版本。
	UPROPERTY(BlueprintReadOnly) int64 Revision = 0;
};

USTRUCT(BlueprintType)
struct FWizardInventoryRequest
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly) int32 Slot = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly) FGuid InstanceId;
	UPROPERTY(BlueprintReadOnly) int64 Revision = -1;
	UPROPERTY(BlueprintReadOnly) bool bEquipment = false;
};

struct FWizardInventoryDropTicket
{
	FWizardInventoryRequest Request;
	FWizardInventoryEntry Entry;
};

DECLARE_MULTICAST_DELEGATE(FWizardInventoryChanged);

UCLASS(ClassGroup=(Wizard), meta=(BlueprintSpawnableComponent))
class WIZARD_API UWizardInventoryComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UWizardInventoryComponent();
	static constexpr int32 Capacity = 20;
	static constexpr int32 EquipmentCapacity = static_cast<int32>(EWizardEquipmentSlot::Count);
	UFUNCTION(BlueprintPure) FWizardInventorySnapshot GetSnapshot() const { return State; }
	UFUNCTION(BlueprintPure) FWizardInventoryRequest MakeRequest(int32 Slot) const;
	UFUNCTION(BlueprintPure) FWizardInventoryRequest MakeEquipmentRequest(EWizardEquipmentSlot Slot) const;
	// 装备/替换完整提交；旧装备回到来源背包格，失败时归属保持不变。
	UFUNCTION(BlueprintCallable) EWizardInventoryResult Equip(FWizardInventoryRequest Request, EWizardEquipmentSlot Slot);
	// 指定背包格可合法交换；未指定时寻找空格，满包则返回NoCapacity。
	UFUNCTION(BlueprintCallable) EWizardInventoryResult Unequip(FWizardInventoryRequest Request, int32 TargetSlot = -1);
	// 接受可容纳的部分；成功仅广播一次，Accepted为准确接收量。
	UFUNCTION(BlueprintCallable) EWizardInventoryResult AddItems(const UWizardItemDefinition* Definition, int32 Quantity, int32& Accepted);
	// 买卖必须全量提交；过期版本、金币/容量不足及非法配置均不改变物品和余额。
	UFUNCTION(BlueprintCallable) EWizardInventoryResult Buy(const UWizardShopCatalog* Catalog, int32 ProductIndex, int32 Quantity, int64 ExpectedRevision);
	UFUNCTION(BlueprintCallable) EWizardInventoryResult Sell(FWizardInventoryRequest Request, int32 Quantity);
	// 当前可购买上限同时受金币、完整接收容量和整数范围限制；不可交易时为0。
	UFUNCTION(BlueprintPure) int32 GetPurchaseLimit(const UWizardShopCatalog* Catalog, int32 ProductIndex) const;
	EWizardInventoryResult CheckPurchase(const UWizardShopCatalog* Catalog, int32 ProductIndex, int32 Quantity, int64 ExpectedRevision) const;
	UFUNCTION(BlueprintCallable) EWizardInventoryResult Move(FWizardInventoryRequest Request, int32 TargetSlot);
	UFUNCTION(BlueprintCallable) EWizardInventoryResult Split(FWizardInventoryRequest Request, int32 Quantity, int32 TargetSlot);
	UFUNCTION(BlueprintCallable) EWizardInventoryResult Use(FWizardInventoryRequest Request);
	// 全量校验身份、定义、数量后替换；无变化不广播。
	UFUNCTION(BlueprintCallable) EWizardInventoryResult RestoreSnapshot(const FWizardInventorySnapshot& Snapshot);
	// 生成可可靠交接的地面对象后扣量；失败不改变库存。
	UFUNCTION(BlueprintCallable) EWizardInventoryResult Discard(FWizardInventoryRequest Request, int32 Quantity);
	EWizardInventoryResult PrepareDrop(FWizardInventoryRequest Request, int32 Quantity, FWizardInventoryDropTicket& Ticket) const;
	// 接收者必须同步完成可靠交接，返回false不得保留生成对象；回调期间背包拒绝重入。
	EWizardInventoryResult CommitDrop(const FWizardInventoryDropTicket& Ticket, TFunctionRef<bool(const FWizardInventoryEntry&)> Receiver);
	FWizardInventoryChanged OnChanged;
private:
	friend class AWizardPlayerState;
	bool IsMutationBlocked() const;
	EWizardInventoryResult PrepareAddItems(const UWizardItemDefinition* Definition, int32 Quantity, TArray<FWizardInventoryEntry>& Slots, int32& Accepted) const;
	EWizardInventoryResult ValidateRequest(const FWizardInventoryRequest& Request) const;
	bool ResolveSnapshot(FWizardInventorySnapshot& Snapshot) const;
	void Commit(TArray<FWizardInventoryEntry>&& Slots);
	void Commit(TArray<FWizardInventoryEntry>&& Slots, TArray<FWizardInventoryEntry>&& Equipment);
	UPROPERTY(Transient) FWizardInventorySnapshot State;
	bool bMutating = false;
};
