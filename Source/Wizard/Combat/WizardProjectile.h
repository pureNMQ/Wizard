#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "WizardProjectile.generated.h"

class UAbilitySystemComponent;
class UWizardCombatConfig;
class USphereComponent;
class UProjectileMovementComponent;
class UNiagaraComponent;
class UWizardSkillDefinition;

UCLASS(Blueprintable)
class WIZARD_API AWizardProjectile : public AActor
{
	GENERATED_BODY()
public:
	AWizardProjectile();
	void InitializeProjectile(UAbilitySystemComponent* Source, const FGameplayEffectSpecHandle& Spec, const UWizardCombatConfig* Config);
	void InitializeSkillProjectile(UAbilitySystemComponent* Source, const FGameplayEffectSpecHandle& Spec, const UWizardSkillDefinition* Definition);
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
	UFUNCTION() void OnHit(UPrimitiveComponent* HitComponent, AActor* Other, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);
	UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UProjectileMovementComponent> Movement;
	// 系统资源和局部变换由弹丸蓝图配置；生命周期始终跟随 Actor。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNiagaraComponent> Trail;
	UPROPERTY() TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	UPROPERTY() FGameplayEffectSpecHandle DamageSpec;
	UPROPERTY() TObjectPtr<const UWizardSkillDefinition> Skill;
	bool bResolved = false;
};
