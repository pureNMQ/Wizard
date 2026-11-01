#include "Inventory/WizardItemDefinition.h"
#include "Engine/AssetManager.h"
#include "Loot/WizardLootSettings.h"
#include "GAS/WizardAttributeSet.h"
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
	if (Type == EWizardItemType::Equipment)
	{
		if (static_cast<uint8>(EquipmentSlot) >= static_cast<uint8>(EWizardEquipmentSlot::Count)) return false;
		TSet<FGameplayAttribute> Attributes;
		for (const FWizardEquipmentBonus& Bonus : EquipmentBonuses)
		{
			const FGameplayAttribute& Attribute = Bonus.Attribute;
			const bool bSupported = Attribute == UWizardAttributeSet::GetMaxHealthAttribute() || Attribute == UWizardAttributeSet::GetMaxManaAttribute()
				|| Attribute == UWizardAttributeSet::GetAttackPowerAttribute() || Attribute == UWizardAttributeSet::GetDefenseAttribute()
				|| Attribute == UWizardAttributeSet::GetMoveSpeedAttribute();
			if (!bSupported || !FMath::IsFinite(Bonus.Magnitude) || Attributes.Contains(Attribute)) return false;
			Attributes.Add(Attribute);
		}
	}
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
		Context.AddError(FText::FromString(TEXT("物品定义无效：需要稳定ID、合法类型和正数堆叠上限；装备上限为1、槽位合法，加成仅允许最大生命、最大法力、攻击力、防御力和移动速度且不可重复；药水需要合法资源和有限正数恢复量。")));
		return EDataValidationResult::Invalid;
	}
	return Parent == EDataValidationResult::Invalid ? Parent : EDataValidationResult::Valid;
}
#endif
