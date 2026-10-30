#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WizardEnemyAssetTools.generated.h"

// 仅由 UE-MCP 调用；编辑器创建资源，运行时不依赖编辑器模块。
UCLASS()
class WIZARD_API UWizardEnemyAssetTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Wizard|Editor") static bool BuildHealthBarAsset();
	UFUNCTION(BlueprintCallable, Category="Wizard|Editor") static bool BuildEnemyAnimationAssets();
};
