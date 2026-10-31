// Wizard 项目：全项目共用的地面表现类，只在项目设置配置一次。
#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WizardLootSettings.generated.h"
class AWizardWorldPickup;
class UTexture2D;

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Wizard Loot"))
class WIZARD_API UWizardLootSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UPROPERTY(Config, EditAnywhere, Category="Loot") TSoftClassPtr<AWizardWorldPickup> PickupClass;
	// 图标加载失败使用引擎默认纹理；表现缺失不改变物品业务结果。
	static UTexture2D* ResolveIcon(const TSoftObjectPtr<UTexture2D>& Icon);
};
