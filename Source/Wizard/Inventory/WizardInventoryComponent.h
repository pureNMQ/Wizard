// Wizard 项目：所有背包写入经完整校验后一次提交，界面只持有带版本的请求。
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/WizardItemDefinition.h"
#include "WizardInventoryComponent.generated.h"

UENUM(BlueprintType)
enum class EWizardInventoryResult : uint8
{
	Success, InvalidSlot, InvalidQuantity, UnknownDefinition, InvalidDefinition, StaleRequest,
	NoCapacity, NotConsumable, FullResource, Dead, EffectFailed, Busy, DropUnavailable, InvalidSnapshot
};

USTRUCT(BlueprintType)
struct FWizardInventoryEntry
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame) FGuid InstanceId;
	UPROPERTY(BlueprintReadOnly, SaveGame) FName DefinitionId;
	UPROPERTY(BlueprintReadOnly, SaveGame) int32 Quantity = 0;
	bool IsEmpty() const { return Quantity == 0 && !InstanceId.IsValid() && DefinitionId.IsNone(); }
	bool operator==(const FWizardInventoryEntry& Other) const { return InstanceId == Other.InstanceId && DefinitionId == Other.DefinitionId && Quantity == Other.Quantity; }
};

USTRUCT(BlueprintType)
struct FWizardInventorySnapshot
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, SaveGame) TArray<FWizardInventoryEntry> Slots;
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
};

USTRUCT(BlueprintType)
struct FWizardInitialItem
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly) FName DefinitionId;
	UPROPERTY(EditDefaultsOnly, meta=(ClampMin="1")) int32 Quantity = 1;
};

struct FWizardInventoryDropTicket
{
	FWizardInventoryRequest Request;
	FWizardInventoryEntry Entry;
};

UCLASS(BlueprintType)
class WIZARD_API UWizardInventoryConfig : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TArray<TObjectPtr<UWizardItemDefinition>> Definitions;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TArray<FWizardInitialItem> InitialItems;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};

DECLARE_MULTICAST_DELEGATE(FWizardInventoryChanged);

UCLASS(ClassGroup=(Wizard), meta=(BlueprintSpawnableComponent))
class WIZARD_API UWizardInventoryComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UWizardInventoryComponent();
	static constexpr int32 Capacity = 20;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory") TObjectPtr<UWizardInventoryConfig> Config;
	// 硬引用目录保证配置可达；图标软引用由定义的资源依赖进入Cook。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory") TArray<TObjectPtr<UWizardItemDefinition>> Catalog;
	UPROPERTY(EditDefaultsOnly, Category="Inventory") TArray<FWizardInitialItem> InitialItems;
	// 初始化整个初始库存；失败不发一半物品，成功后重复调用不再发放。
	EWizardInventoryResult InitializeInventory();
	UFUNCTION(BlueprintPure) FWizardInventorySnapshot GetSnapshot() const { return State; }
	UFUNCTION(BlueprintPure) const UWizardItemDefinition* FindDefinition(FName Id) const;
	UFUNCTION(BlueprintPure) FWizardInventoryRequest MakeRequest(int32 Slot) const;
	// 接受可容纳的部分；成功仅广播一次，Accepted为准确接收量。
	UFUNCTION(BlueprintCallable) EWizardInventoryResult AddItems(FName Id, int32 Quantity, int32& Accepted);
	UFUNCTION(BlueprintCallable) EWizardInventoryResult Move(FWizardInventoryRequest Request, int32 TargetSlot);
	UFUNCTION(BlueprintCallable) EWizardInventoryResult Split(FWizardInventoryRequest Request, int32 Quantity, int32 TargetSlot);
	UFUNCTION(BlueprintCallable) EWizardInventoryResult Use(FWizardInventoryRequest Request);
	// 全量校验身份、定义、数量后替换；无变化不广播。
	UFUNCTION(BlueprintCallable) EWizardInventoryResult RestoreSnapshot(const FWizardInventorySnapshot& Snapshot);
	EWizardInventoryResult PrepareDrop(FWizardInventoryRequest Request, int32 Quantity, FWizardInventoryDropTicket& Ticket) const;
	// 接收者必须同步完成可靠交接，返回false不得保留生成对象；回调期间背包拒绝重入。
	EWizardInventoryResult CommitDrop(const FWizardInventoryDropTicket& Ticket, TFunctionRef<bool(const FWizardInventoryEntry&)> Receiver);
	FWizardInventoryChanged OnChanged;
private:
	EWizardInventoryResult ValidateRequest(const FWizardInventoryRequest& Request) const;
	bool IsCatalogValid() const;
	bool IsSnapshotValid(const FWizardInventorySnapshot& Snapshot) const;
	void Commit(TArray<FWizardInventoryEntry>&& Slots);
	UPROPERTY(Transient) FWizardInventorySnapshot State;
	bool bInitialized = false;
	bool bMutating = false;
};
