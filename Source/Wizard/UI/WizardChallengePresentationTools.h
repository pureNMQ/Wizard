#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WizardChallengePresentationTools.generated.h"

// 仅由 UE-MCP 显式调用编辑器装配入口。
UCLASS()
class WIZARD_API UWizardChallengePresentationTools : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Wizard|Editor") static bool BuildChallengeBannerAsset();
};
