// Wizard 项目：战斗数值配置。
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WizardCombatConfig.generated.h"

class UGameplayAbility;

UCLASS(BlueprintType)
class WIZARD_API UWizardCombatConfig : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat")
	TSubclassOf<UGameplayAbility> BasicAttackAbilityClass;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation") TObjectPtr<class UWizardCombatPresentation> Presentation;
	bool IsConfigurationValid() const;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat", meta=(ClampMin="1"))
	float ProjectileSpeed = 1600.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat", meta=(ClampMin="0.1"))
	float ProjectileLifetime = 2.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Combat")
	FVector MuzzleOffset = FVector(65.0f, 0.0f, 25.0f);
};
