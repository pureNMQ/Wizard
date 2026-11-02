// Wizard 项目：只读技能定义与持久搭配快照。
#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "WizardSkillDefinition.generated.h"
class UGameplayAbility;
class UAnimMontage;
class UTexture2D;
class UNiagaraSystem;
class UParticleSystem;
class USoundBase;

UENUM(BlueprintType)
enum class EWizardSkillBehavior : uint8
{
	Projectile, Area, Blink, Shield, Heal
};

UENUM(BlueprintType)
enum class EWizardSkillResult : uint8
{
	Success, InvalidSlot, UnknownSkill, DuplicateSkill, InvalidConfiguration, NotAtAlchemy,
	Dead, Busy, NotEnoughMana, CoolingDown, InvalidTarget, FullHealth, ActivationFailed
};

USTRUCT(BlueprintType)
struct FWizardLoadoutSnapshot
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FName> Slots;
};

UCLASS(BlueprintType)
class WIZARD_API UWizardSkillDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName SkillId;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation") TObjectPtr<class UWizardCombatPresentation> Presentation;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine=true)) FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EWizardSkillBehavior Behavior = EWizardSkillBehavior::Projectile;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<UGameplayAbility> AbilityClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTag CooldownTag;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UTexture2D> Icon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UAnimMontage> Montage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UNiagaraSystem> ProjectileFX;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UNiagaraSystem> ImpactFX;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UParticleSystem> CascadeFX;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<USoundBase> CastSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<USoundBase> ImpactSound;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float ManaCost = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.05")) float Cooldown = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float Power = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) float Range = 900.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) float Radius = 220.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.05")) float Duration = 3.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.1",ClampMax="1")) float SpeedMultiplier = 1.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) float ProjectileSpeed = 1600.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.05")) float EffectLifetime = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector EffectScale = FVector(0.5f);
	bool IsConfigurationValid() const;
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};

UCLASS(BlueprintType)
class WIZARD_API UWizardSkillCatalog : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<TObjectPtr<UWizardSkillDefinition>> Skills;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FWizardLoadoutSnapshot InitialLoadout;
	const UWizardSkillDefinition* Find(FName Id) const;
	bool IsConfigurationValid() const;
};
