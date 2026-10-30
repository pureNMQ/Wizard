// Wizard 项目：敌人战斗与表现配置。
#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WizardEnemyConfig.generated.h"

UCLASS(BlueprintType)
class WIZARD_API UWizardEnemyConfig : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<class UBehaviorTree> BehaviorTree;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<class USkeletalMesh> SkeletalMesh;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TSubclassOf<class UAnimInstance> AnimClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<class UAnimMontage> AttackMontage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<class UAnimSequence> DeathAnimation;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector MeshOffset = FVector(0, 0, -88);
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FRotator MeshRotation = FRotator(0, -90, 0);
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector MeshScale = FVector(1);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) float Health = 60;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float AttackPower = 12;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0")) float Defense = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) float MoveSpeed = 240;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="80")) float AttackRange = 160;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.1")) float AttackInterval = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="0.01")) float AttackWindup = .35f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bRanged = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) float ProjectileSpeed = 700;
};
