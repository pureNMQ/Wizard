// Wizard 项目：挑战独占波次生命周期，场景只配置位置与敌人数据。
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TimerHandle.h"
#include "WizardChallengeArena.generated.h"

class AWizardEnemy;
class AWizardPlayerState;
class UBoxComponent;
struct FOnAttributeChangeData;

USTRUCT(BlueprintType)
struct FWizardChallengeSpawn
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TSubclassOf<AWizardEnemy> EnemyClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="32")) int32 Count = 1;
};

USTRUCT(BlueprintType)
struct FWizardChallengeWave
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FWizardChallengeSpawn> Spawns;
};

UENUM(BlueprintType)
enum class EWizardChallengeState : uint8 { Ready, Fighting, BetweenWaves, Returning, ConfigurationFailed };

UCLASS(Blueprintable)
class WIZARD_API AWizardChallengeArena : public AActor
{
	GENERATED_BODY()
public:
	AWizardChallengeArena();
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation") TObjectPtr<class UWizardCombatPresentation> Presentation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Challenge") float ArenaRadius = 1780.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Challenge") float EntryInset = 140.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Challenge") FVector CampReturnLocation = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Challenge") FRotator CampReturnRotation = FRotator::ZeroRotator;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Challenge") TArray<FWizardChallengeWave> Waves;
	// 相对Actor圆心的出生点，Z为地面探测起点；正式生成按地面抬升胶囊半高。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Challenge") TArray<FVector> SpawnOffsets;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Challenge") float WaveDelay = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Challenge", meta=(ClampMin="0.1")) float ReturnDelay = 2.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Challenge") TObjectPtr<class UWizardBarrierVisualComponent> BarrierVisual;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Challenge") EWizardChallengeState State = EWizardChallengeState::Ready;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Challenge") int32 CurrentWave = INDEX_NONE;
	UFUNCTION(BlueprintPure, Category="Challenge") int32 GetLivingEnemyCount() const { return LivingEnemies.Num(); }
	UFUNCTION(BlueprintCallable, Category="Challenge") bool TryStartChallenge(AWizardPlayerState* Player);
	UFUNCTION(BlueprintPure, Category="Challenge") bool IsConfigurationValid() const;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	void MonitorPlayer();
	void SpawnNextWave();
	void BuildBarrier();
	void SetBarrierEnabled(bool bEnabled);
	void FinishChallenge();
	void ReturnToCamp();
	void ShowAnnouncement(const FText& Text, const FText& Detail, const FLinearColor& Color, float Duration);
	void HandlePlayerHealth(const FOnAttributeChangeData& Data);
	UFUNCTION() void HandleEnemyDefeated(AWizardEnemy* Enemy);
	UFUNCTION() void HandleEnemyDestroyed(AActor* Enemy);
	void ClearRound();
	UPROPERTY() TArray<TObjectPtr<UBoxComponent>> BarrierSegments;
	UPROPERTY() TArray<TObjectPtr<AWizardEnemy>> RoundEnemies;
	TSet<TWeakObjectPtr<AWizardEnemy>> LivingEnemies;
	TWeakObjectPtr<AWizardPlayerState> ActivePlayer;
	FDelegateHandle HealthHandle;
	FTimerHandle MonitorTimer, WaveTimer, ReturnTimer;
	bool bRoundWon = false;
};
