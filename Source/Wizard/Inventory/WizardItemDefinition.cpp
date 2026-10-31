#include "Inventory/WizardItemDefinition.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

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
