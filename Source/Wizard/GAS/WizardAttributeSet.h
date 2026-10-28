// Wizard 项目：GAS 是战斗数值的唯一权威来源。
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "WizardAttributeSet.generated.h"

#define WIZARD_ATTRIBUTE_ACCESSORS(Property) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(UWizardAttributeSet, Property) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(Property) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(Property) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(Property)

UCLASS()
class WIZARD_API UWizardAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
public:
	UWizardAttributeSet();
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	UPROPERTY(BlueprintReadOnly, Category="Attributes") FGameplayAttributeData Health;
	WIZARD_ATTRIBUTE_ACCESSORS(Health)
	UPROPERTY(BlueprintReadOnly, Category="Attributes") FGameplayAttributeData MaxHealth;
	WIZARD_ATTRIBUTE_ACCESSORS(MaxHealth)
	UPROPERTY(BlueprintReadOnly, Category="Attributes") FGameplayAttributeData Mana;
	WIZARD_ATTRIBUTE_ACCESSORS(Mana)
	UPROPERTY(BlueprintReadOnly, Category="Attributes") FGameplayAttributeData MaxMana;
	WIZARD_ATTRIBUTE_ACCESSORS(MaxMana)
	UPROPERTY(BlueprintReadOnly, Category="Attributes") FGameplayAttributeData AttackPower;
	WIZARD_ATTRIBUTE_ACCESSORS(AttackPower)
	UPROPERTY(BlueprintReadOnly, Category="Attributes") FGameplayAttributeData Defense;
	WIZARD_ATTRIBUTE_ACCESSORS(Defense)
	// 伤害执行输出的瞬时元属性，不作为另一个生命状态。
	UPROPERTY() FGameplayAttributeData IncomingDamage;
	WIZARD_ATTRIBUTE_ACCESSORS(IncomingDamage)
};
