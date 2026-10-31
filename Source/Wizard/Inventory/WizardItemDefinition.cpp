#include "Inventory/WizardItemDefinition.h"
#include "Engine/AssetManager.h"
#include "Loot/WizardLootSettings.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

FPrimaryAssetId UWizardItemDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("WizardItem"), ItemId);
}

const UWizardItemDefinition* UWizardItemDefinition::Resolve(FName Id)
{
	if (Id.IsNone()) return nullptr;
	const UWizardItemDefinition* Definition = Cast<UWizardItemDefinition>(UAssetManager::Get().GetPrimaryAssetPath(FPrimaryAssetId(TEXT("WizardItem"), Id)).TryLoad());
	return Definition && Definition->ItemId == Id ? Definition : nullptr;
}

UTexture2D* UWizardItemDefinition::GetIconTexture() const
{
	return UWizardLootSettings::ResolveIcon(Icon);
}

bool UWizardItemDefinition::IsConfigurationValid() const
{
	const bool bKnownType = Type == EWizardItemType::Equipment || Type == EWizardItemType::Consumable || Type == EWizardItemType::Material;
	const bool bKnownResource = PotionResource == EWizardPotionResource::None || PotionResource == EWizardPotionResource::Health || PotionResource == EWizardPotionResource::Mana;
	return bKnownType && bKnownResource && !ItemId.IsNone() && GetStackLimit() > 0 && (Type != EWizardItemType::Equipment || MaxStack == 1) &&
		(PotionResource == EWizardPotionResource::None ||
		(Type == EWizardItemType::Consumable && FMath::IsFinite(PotionMagnitude) && PotionMagnitude > 0.f));
}

#if WITH_EDITOR
EDataValidationResult UWizardItemDefinition::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult Parent = Super::IsDataValid(Context);
	if (!IsConfigurationValid())
	{
		Context.AddError(FText::FromString(TEXT("物品定义无效：需要稳定ID、合法类型和正数堆叠上限；装备上限必须为1，药水必须有合法资源及有限正数恢复量。")));
		return EDataValidationResult::Invalid;
	}
	return Parent == EDataValidationResult::Invalid ? Parent : EDataValidationResult::Valid;
}
#endif
