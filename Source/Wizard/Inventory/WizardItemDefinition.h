// Wizard 项目：只读物品配置与运行身份、存放位置分离。
#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WizardItemDefinition.generated.h"
class UTexture2D;

UENUM(BlueprintType)
enum class EWizardItemType : uint8 { Equipment, Consumable, Material };
UENUM(BlueprintType)
enum class EWizardPotionResource : uint8 { None, Health, Mana };

UCLASS(BlueprintType)
class WIZARD_API UWizardItemDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FName ItemId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FText DisplayName;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FText Description;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) EWizardItemType Type = EWizardItemType::Material;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="1")) int32 MaxStack = 20;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) EWizardPotionResource PotionResource = EWizardPotionResource::None;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0")) float PotionMagnitude = 0.f;
	int32 GetStackLimit() const { return Type == EWizardItemType::Equipment ? 1 : MaxStack; }
	bool IsConfigurationValid() const;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
