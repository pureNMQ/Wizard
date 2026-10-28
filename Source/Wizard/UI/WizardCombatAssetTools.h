#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WizardCombatAssetTools.generated.h"
// 仅编辑器实现，用于可复跑地生成输入和布局资产；成品无编辑器依赖。
UCLASS()
class WIZARD_API UWizardCombatAssetTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Wizard|Editor") static bool BuildCombatAssets();
};
