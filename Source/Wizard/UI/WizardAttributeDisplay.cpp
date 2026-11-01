#include "UI/WizardAttributeDisplay.h"
#include "GAS/WizardAttributeSet.h"

FString WizardAttributeDisplay::GetLabel(const FGameplayAttribute& Attribute)
{
	if (Attribute == UWizardAttributeSet::GetHealthAttribute()) return TEXT("生命");
	if (Attribute == UWizardAttributeSet::GetMaxHealthAttribute()) return TEXT("最大生命");
	if (Attribute == UWizardAttributeSet::GetManaAttribute()) return TEXT("法力");
	if (Attribute == UWizardAttributeSet::GetMaxManaAttribute()) return TEXT("最大法力");
	if (Attribute == UWizardAttributeSet::GetAttackPowerAttribute()) return TEXT("攻击力");
	if (Attribute == UWizardAttributeSet::GetDefenseAttribute()) return TEXT("防御力");
	if (Attribute == UWizardAttributeSet::GetMoveSpeedAttribute()) return TEXT("移动速度");
	if (Attribute == UWizardAttributeSet::GetShieldAttribute()) return TEXT("护盾");
	return Attribute.GetName();
}
