// Wizard 项目：磁盘快照仅持有值和稳定定义ID，不保存Actor、GE或运行时资源引用。
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Inventory/WizardInventoryComponent.h"
#include "Skills/WizardSkillDefinition.h"
#include "WizardSaveGame.generated.h"

UCLASS()
class WIZARD_API UWizardSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
	static constexpr int32 CurrentVersion = 1;
	UPROPERTY(SaveGame) int32 SchemaVersion = CurrentVersion;
	UPROPERTY(SaveGame) int32 Gold = 0;
	UPROPERTY(SaveGame) FWizardInventorySnapshot Inventory;
	UPROPERTY(SaveGame) FWizardLoadoutSnapshot Loadout;
};

// 单机Windows存储边界。校验封套先于UE对象反序列化，阻止截断/损坏数据进入对象读取。
namespace WizardSaveStorage
{
	bool Encode(UWizardSaveGame* Snapshot, TArray<uint8>& Bytes);
	UWizardSaveGame* Decode(const TArray<uint8>& Bytes);
	bool Read(const FString& Path, TArray<uint8>& Bytes);
	// 完整临时文件关闭后才替换正式文件；保留上一份成功快照为.bak。
	bool Write(const FString& Path, const TArray<uint8>& Bytes);
}
