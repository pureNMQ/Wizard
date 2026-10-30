#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WizardBarrierAssetTools.generated.h"

// 仅由UE-MCP显式调用生成资源，运行时不装配材质图。
UCLASS()
class WIZARD_API UWizardBarrierAssetTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Wizard|Editor") static bool BuildBarrierMaterial();
};
