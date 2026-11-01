// Wizard 项目：只读物品配置与运行身份、存放位置分离。
#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AttributeSet.h"
#include "WizardItemDefinition.generated.h"
class UTexture2D;
class UStaticMesh;

UENUM(BlueprintType)
enum class EWizardEquipmentSlot : uint8 { Hat, Clothes, Boots, Staff, Count UMETA(Hidden) };

USTRUCT(BlueprintType)
struct FWizardEquipmentBonus
{
	GENERATED_BODY()
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) FGameplayAttribute Attribute;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) float Magnitude = 0.f;
};

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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) bool bTradable = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="1", EditCondition="bTradable")) int32 SellPrice = 1;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) EWizardItemType Type = EWizardItemType::Material;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="1")) int32 MaxStack = 20;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) TSoftObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly) EWizardPotionResource PotionResource = EWizardPotionResource::None;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(ClampMin="0")) float PotionMagnitude = 0.f;
	int32 GetStackLimit() const { return Type == EWizardItemType::Equipment ? 1 : MaxStack; }
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(EditCondition="Type == EWizardItemType::Equipment")) EWizardEquipmentSlot EquipmentSlot = EWizardEquipmentSlot::Hat;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(EditCondition="Type == EWizardItemType::Equipment")) TArray<FWizardEquipmentBonus> EquipmentBonuses;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(EditCondition="Type == EWizardItemType::Equipment")) TSoftObjectPtr<UStaticMesh> StaffMesh;
	bool IsConfigurationValid() const;
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	static const UWizardItemDefinition* Resolve(FName Id);
	class UTexture2D* GetIconTexture() const;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};
