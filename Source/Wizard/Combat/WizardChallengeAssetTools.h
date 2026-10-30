#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WizardChallengeAssetTools.generated.h"

// 由 UE-MCP 显式调用，不在加载或运行游戏时修改资产。
UCLASS()
class WIZARD_API UWizardChallengeAssetTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Wizard|Editor") static bool BuildEnemyDefinitions();
	UFUNCTION(BlueprintCallable, Category="Wizard|Editor") static bool ConfigureChallengeMap();
};
