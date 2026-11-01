// Wizard 项目：目录仅配置商品与单价，运行物品及金币由玩家物品域持有。
#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Inventory/WizardItemDefinition.h"
#include "WizardShopCatalog.generated.h"

USTRUCT(BlueprintType)
struct FWizardShopProduct
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TObjectPtr<const UWizardItemDefinition> Definition;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="1")) int32 BuyPrice = 1;
};

UCLASS(BlueprintType)
class WIZARD_API UWizardShopCatalog : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TArray<FWizardShopProduct> Products;
	// 首版仅支持无限库存；不支持的规则在校验时明确拒绝，避免静默忽略配置。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) bool bInfiniteStock = true;
	bool IsConfigurationValid() const;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
