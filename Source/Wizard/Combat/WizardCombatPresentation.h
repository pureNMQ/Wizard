// Wizard 项目：表现只消费结算结果，不拥有战斗数值。
#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayCueNotify_Static.h"
#include "GameplayCueNotify_Actor.h"
#include "WizardCombatPresentation.generated.h"
class UNiagaraSystem;
class UNiagaraComponent;
class UParticleSystem;
class USoundBase;
class UFXSystemComponent;
class UAbilitySystemComponent;
UENUM(BlueprintType)
enum class EWizardCombatStage : uint8
{
	Cast,
	Impact,
	WorldImpact,
	Windup,
	Status,
	Hurt,
	Death,
	ShieldHit,
	ShieldBreak,
	Expire,
	Departure,
	Arrival,
	Spawn,
	Wave,
	Victory,
	Defeat,
	BarrierOpen,
	BarrierClose
};
USTRUCT(BlueprintType)
struct FWizardCombatStageStyle
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere) TObjectPtr<UNiagaraSystem> Niagara;
	UPROPERTY(EditAnywhere) TObjectPtr<UParticleSystem> Cascade;
	UPROPERTY(EditAnywhere) TObjectPtr<USoundBase> Sound;
	UPROPERTY(EditAnywhere) FVector Offset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere) FVector Scale = FVector(1);
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.01")) float Lifetime = .65f;
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", ClampMax = "2")) float Volume = .6f;
};
UCLASS(BlueprintType)
class WIZARD_API UWizardCombatPresentation : public UDataAsset
{
	GENERATED_BODY()
  public:
	UPROPERTY(EditAnywhere) TMap<EWizardCombatStage, FWizardCombatStageStyle> Stages;
	UPROPERTY(EditAnywhere) TObjectPtr<UNiagaraSystem> Projectile;
	UPROPERTY(EditAnywhere) FVector ProjectileScale = FVector(.5f);
	UPROPERTY(EditAnywhere) TObjectPtr<class UMaterialInterface> FlashMaterial;
	UPROPERTY(EditAnywhere) TObjectPtr<class UMaterialInterface> TargetRingMaterial;
	UPROPERTY(EditAnywhere) TObjectPtr<class UMaterialInterface> ShieldMaterial;
	// 盾体各轴半径相对胶囊半高的倍率，为跑动姿态保留包裹余量。
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0.1")) FVector ShieldRadiusScale = FVector(1.25f, 1.25f, 1.35f);
	UPROPERTY(EditAnywhere) TMap<FName, FLinearColor> ColorOverrides;
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", ClampMax = "0.08")) float HitStopSeconds = 0;
	UPROPERTY(EditAnywhere, meta = (ClampMin = "0", ClampMax = "3")) int32 ImpactLevel = 1;
};
USTRUCT()
struct FWizardCombatResult
{
	GENERATED_BODY()
	UPROPERTY() TObjectPtr<AActor> Source;
	UPROPERTY() TObjectPtr<AActor> Target;
	UPROPERTY() TObjectPtr<const UWizardCombatPresentation> Presentation;
	FVector Location = FVector::ZeroVector;
	float HealthLost = 0;
	float Absorbed = 0;
	float Healed = 0;
	bool bKilled = false;
	bool bShieldBroken = false;
};
namespace WizardCombat
{
	WIZARD_API const UWizardCombatPresentation* ResolvePresentation(const UObject* Source);
	WIZARD_API void ApplyColors(UNiagaraComponent* Component, const UWizardCombatPresentation* Profile);
	WIZARD_API FGameplayTag BurstTag();
	WIZARD_API FGameplayTag WindupTag();
	WIZARD_API FGameplayTag SlowTag();
	WIZARD_API void Burst(AActor* Target, const UWizardCombatPresentation* Profile, EWizardCombatStage Stage, FVector Location,
	                      FVector Normal = FVector::UpVector);
	WIZARD_API UFXSystemComponent* SpawnStyle(AActor* Target, const UWizardCombatPresentation* Profile, EWizardCombatStage Stage,
	                                          FVector Location, bool bAttached = false, FVector Normal = FVector::UpVector);
	WIZARD_API void Report(const FWizardCombatResult& Result);
} // namespace WizardCombat
UCLASS(Blueprintable)
class WIZARD_API UWizardCombatBurstCue : public UGameplayCueNotify_Static
{
	GENERATED_BODY()
  public:
	UWizardCombatBurstCue();
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const override;
};
UCLASS(Blueprintable)
class WIZARD_API AWizardCombatStatusCue : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()
  public:
	AWizardCombatStatusCue();
	virtual bool OnActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

  private:
	UPROPERTY() TObjectPtr<UFXSystemComponent> Effect;
	UPROPERTY() TObjectPtr<class UStaticMeshComponent> TargetRing;
	UPROPERTY() TObjectPtr<class UStaticMesh> RingMesh;
};
