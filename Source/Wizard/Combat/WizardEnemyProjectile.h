// Wizard 项目：敌人弹丸仅对存活玩家结算，碰到场景立即消耗。
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "WizardEnemyProjectile.generated.h"
UCLASS()
class WIZARD_API AWizardEnemyProjectile : public AActor
{
	GENERATED_BODY()
public:
	AWizardEnemyProjectile();
	void Initialize(class UAbilitySystemComponent* Source, const FGameplayEffectSpecHandle& Spec, float Speed);
	// 独立于发射者Actor的生命周期；敌人销毁后仍可由所属挑战清理。
	bool BelongsToChallenge(const class AWizardChallengeArena* Challenge) const;
protected:
	virtual void BeginPlay() override;
private:
	UFUNCTION() void OnHit(class UPrimitiveComponent* HitComponent, AActor* Other, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);
	UPROPERTY(VisibleAnywhere) TObjectPtr<class USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UProjectileMovementComponent> Movement;
	UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Visual;
	FGameplayEffectSpecHandle DamageSpec;
	TWeakObjectPtr<class UAbilitySystemComponent> SourceASC;
	TWeakObjectPtr<class AWizardChallengeArena> SourceChallenge;
	bool bResolved = false;
};
