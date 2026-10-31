#include "Challenge/WizardChallengeArena.h"
#include "AbilitySystemComponent.h"
#include "Combat/WizardEnemy.h"
#include "Combat/WizardEnemyProjectile.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GAS/WizardAttributeSet.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "WizardPlayerState.h"
#include "WizardPlayerController.h"
#include "UI/WizardUIRoot.h"
#include "Challenge/WizardBarrierVisualComponent.h"
#include "Skills/WizardSkillLoadoutComponent.h"
#include "Loot/WizardWorldPickup.h"

AWizardChallengeArena::AWizardChallengeArena()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("ArenaRoot")));
	BarrierVisual = CreateDefaultSubobject<UWizardBarrierVisualComponent>(TEXT("BarrierVisual"));
	BarrierVisual->SetupAttachment(RootComponent);
}

bool AWizardChallengeArena::IsConfigurationValid() const
{
	if (!FMath::IsFinite(ArenaRadius) || ArenaRadius <= 300.f || EntryInset < 100.f || EntryInset >= ArenaRadius
		|| Waves.IsEmpty() || SpawnOffsets.IsEmpty() || CampReturnLocation.ContainsNaN()
		|| FVector::Dist2D(CampReturnLocation, GetActorLocation()) <= ArenaRadius + 100.f) return false;
	for (const FWizardChallengeWave& Wave : Waves)
	{
		if (Wave.Spawns.IsEmpty()) return false;
		for (const FWizardChallengeSpawn& Spawn : Wave.Spawns)
		{
			if (!Spawn.EnemyClass || Spawn.Count < 1 || Spawn.Count > 32) return false;
			if (Spawn.EnemyClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)
				|| !Spawn.EnemyClass->GetDefaultObject<AWizardEnemy>()->IsConfigurationValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("波次敌人配置无效：%s；请检查该蓝图的战斗数值与行为树"), *Spawn.EnemyClass->GetPathName());
				return false;
			}
		}
	}
	for (const FVector& Offset : SpawnOffsets)
	{
		if (Offset.ContainsNaN() || Offset.Size2D() > ArenaRadius - 120.f) return false;
	}
	return FMath::IsFinite(WaveDelay) && WaveDelay >= 0.f && FMath::IsFinite(ReturnDelay) && ReturnDelay >= .1f;
}

void AWizardChallengeArena::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority()) return;
	BuildBarrier();
	BarrierVisual->BuildRing(ArenaRadius);
	BarrierVisual->SetVisibility(false);
	if (!IsConfigurationValid())
	{
		State = EWizardChallengeState::ConfigurationFailed;
		UE_LOG(LogTemp, Error, TEXT("挑战区域配置无效：%s；保持入口开放"), *GetName());
		return;
	}
	GetWorldTimerManager().SetTimer(MonitorTimer, this, &ThisClass::MonitorPlayer, .1f, true);
}

void AWizardChallengeArena::BuildBarrier()
{
	// 稍微重叠的切向盒覆盖整圈，禁止跳出或闪现穿过；不动态重建导航。
	constexpr int32 SegmentCount = 64;
	const float HalfLength = ArenaRadius * FMath::Tan(PI / SegmentCount) + 6.f;
	for (int32 Index = 0; Index < SegmentCount; ++Index)
	{
		const float Angle = 2.f * PI * Index / SegmentCount;
		UBoxComponent* Box = NewObject<UBoxComponent>(this);
		Box->SetupAttachment(RootComponent);
		Box->SetRelativeLocation(FVector(FMath::Cos(Angle) * ArenaRadius, FMath::Sin(Angle) * ArenaRadius, 250.f));
		Box->SetRelativeRotation(FRotator(0.f, FMath::RadiansToDegrees(Angle), 0.f));
		Box->SetBoxExtent(FVector(35.f, HalfLength, 500.f));
		Box->SetCollisionObjectType(ECC_WorldStatic);
		Box->SetCollisionResponseToAllChannels(ECR_Ignore);
		Box->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Box->SetCanEverAffectNavigation(false);
		Box->SetHiddenInGame(true);
		Box->RegisterComponent();
		BarrierSegments.Add(Box);
	}
}

void AWizardChallengeArena::SetBarrierEnabled(bool bEnabled)
{
	for (UBoxComponent* Box : BarrierSegments) Box->SetCollisionEnabled(bEnabled ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
	BarrierVisual->SetVisibility(bEnabled);
}

void AWizardChallengeArena::MonitorPlayer()
{
	if (State == EWizardChallengeState::Fighting || State == EWizardChallengeState::BetweenWaves)
	{
		if (!ActivePlayer.IsValid() || !IsValid(ActivePlayer->GetAbilitySystemComponent()->GetAvatarActor())) FinishChallenge();
		return;
	}
	if (State != EWizardChallengeState::Ready) return;
	APlayerController* Controller = UGameplayStatics::GetPlayerController(this, 0);
	if (Controller) TryStartChallenge(Controller->GetPlayerState<AWizardPlayerState>());
}

bool AWizardChallengeArena::TryStartChallenge(AWizardPlayerState* Player)
{
	if (!HasAuthority() || State != EWizardChallengeState::Ready || !Player || Player->IsInChallenge() || !IsConfigurationValid()
		|| Player->GetAttributes()->GetHealth() <= 0.f || GetWorld()->IsPaused()) return false;
	AActor* Avatar = Player->GetAbilitySystemComponent()->GetAvatarActor();
	if (!Avatar || FVector::Dist2D(Avatar->GetActorLocation(), GetActorLocation()) > ArenaRadius - EntryInset
		|| FMath::Abs(Avatar->GetActorLocation().Z - GetActorLocation().Z) > 500.f) return false;
	ActivePlayer = Player;
	// 本轮结束保留战利品供结果展示期间拾取，下次挑战才清除上一轮地面余量。
	TArray<AWizardWorldPickup*> OldDrops;
	for (TActorIterator<AWizardWorldPickup> It(GetWorld()); It; ++It)
	{
		if (It->BelongsToChallenge(this)) OldDrops.Add(*It);
	}
	for (AWizardWorldPickup* Pickup : OldDrops) Pickup->Destroy();
	bRoundWon = false;
	Player->SetInChallenge(true);
	HealthHandle = Player->GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetHealthAttribute()).AddUObject(this, &ThisClass::HandlePlayerHealth);
	CurrentWave = INDEX_NONE;
	State = EWizardChallengeState::BetweenWaves;
	SetBarrierEnabled(true);
	ShowAnnouncement(FText::FromString(TEXT("战斗开始")), FText::FromString(TEXT("击败三波敌人")), FLinearColor(.35f, .85f, 1.f), 1.8f);
	SpawnNextWave();
	return State == EWizardChallengeState::Fighting;
}

void AWizardChallengeArena::SpawnNextWave()
{
	if (State != EWizardChallengeState::BetweenWaves || !ActivePlayer.IsValid()) return;
	++CurrentWave;
	if (!Waves.IsValidIndex(CurrentWave)) { FinishChallenge(); return; }
	State = EWizardChallengeState::Fighting;
	int32 SpawnIndex = 0;
	for (const FWizardChallengeSpawn& Entry : Waves[CurrentWave].Spawns)
	{
		for (int32 Index = 0; Index < Entry.Count; ++Index)
		{
			FVector Offset = SpawnOffsets[SpawnIndex % SpawnOffsets.Num()];
			const int32 Layer = SpawnIndex++ / SpawnOffsets.Num();
			if (Layer > 0)
			{
				const float Angle = Layer * 2.4f;
				Offset += FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * (150.f * FMath::Sqrt(static_cast<float>(Layer)));
				const FVector Horizontal = FVector(Offset.X, Offset.Y, 0.f).GetClampedToMaxSize(ArenaRadius - 180.f);
				Offset.X = Horizontal.X;
				Offset.Y = Horizontal.Y;
			}
			FVector Location = GetActorLocation() + Offset;
			FHitResult Ground;
			FCollisionQueryParams Query(SCENE_QUERY_STAT(ChallengeSpawn), false, this);
			for (AWizardEnemy* Enemy : RoundEnemies) Query.AddIgnoredActor(Enemy);
			if (GetWorld()->LineTraceSingleByChannel(Ground, Location + FVector(0,0,1000), Location - FVector(0,0,2000), ECC_WorldStatic, Query))
			{
				Location.Z = Ground.ImpactPoint.Z + Entry.EnemyClass->GetDefaultObject<AWizardEnemy>()->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 3.f;
			}
			FActorSpawnParameters Params;
			Params.Owner = this;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
			AWizardEnemy* Enemy = GetWorld()->SpawnActor<AWizardEnemy>(Entry.EnemyClass, Location, FRotator::ZeroRotator, Params);
			if (!Enemy)
			{
				UE_LOG(LogTemp, Error, TEXT("挑战生成敌人失败，终止本轮并恢复营地：波%d"), CurrentWave + 1);
				FinishChallenge();
				return;
			}
			RoundEnemies.Add(Enemy);
			Enemy->SourceChallenge = this;
			LivingEnemies.Add(Enemy);
			Enemy->OnEnemyDefeated.AddDynamic(this, &ThisClass::HandleEnemyDefeated);
			Enemy->OnDestroyed.AddDynamic(this, &ThisClass::HandleEnemyDestroyed);
		}
	}
	UE_LOG(LogTemp, Display, TEXT("挑战第%d波开始：%d个敌人"), CurrentWave + 1, LivingEnemies.Num());
}

void AWizardChallengeArena::HandleEnemyDefeated(AWizardEnemy* Enemy)
{
	if (State != EWizardChallengeState::Fighting || LivingEnemies.Remove(Enemy) == 0) return;
	if (LivingEnemies.IsEmpty())
	{
		if (CurrentWave + 1 >= Waves.Num())
		{
			bRoundWon = true;
			State = EWizardChallengeState::BetweenWaves;
			ReturnTimer = GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::FinishChallenge);
			return;
		}
		State = EWizardChallengeState::BetweenWaves;
		GetWorldTimerManager().SetTimer(WaveTimer, this, &ThisClass::SpawnNextWave, FMath::Max(.01f, WaveDelay), false);
	}
}

void AWizardChallengeArena::HandleEnemyDestroyed(AActor* Enemy)
{
	// 外部删除活怪属于失败，不能静默卡在战斗中或视为胜利。
	if (State == EWizardChallengeState::Fighting && LivingEnemies.Contains(Cast<AWizardEnemy>(Enemy))) FinishChallenge();
}

void AWizardChallengeArena::HandlePlayerHealth(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue <= 0.f && State != EWizardChallengeState::Returning && !ReturnTimer.IsValid())
	{
		// 避免在伤害GE属性回调栈中恢复生命，下一帧完成回营地事务。
		ReturnTimer = GetWorldTimerManager().SetTimerForNextTick(this, &ThisClass::FinishChallenge);
	}
}

void AWizardChallengeArena::ClearRound()
{
	// 按独立挑战归属清理指定弹丸类型；发射者死亡或GC不影响归属，也不匹配空Owner。
	TArray<AWizardEnemyProjectile*> Projectiles;
	for (TActorIterator<AWizardEnemyProjectile> It(GetWorld()); It; ++It)
	{
		if (It->BelongsToChallenge(this)) Projectiles.Add(*It);
	}
	for (AWizardEnemyProjectile* Projectile : Projectiles) Projectile->Destroy();
	for (AWizardEnemy* Enemy : RoundEnemies)
	{
		if (!IsValid(Enemy)) continue;
		Enemy->OnEnemyDefeated.RemoveAll(this);
		Enemy->OnDestroyed.RemoveAll(this);
		Enemy->Destroy();
	}
	RoundEnemies.Empty();
	LivingEnemies.Empty();
}

void AWizardChallengeArena::FinishChallenge()
{
	if (State == EWizardChallengeState::Returning || State == EWizardChallengeState::Ready) return;
	State = EWizardChallengeState::Returning;
	GetWorldTimerManager().ClearTimer(WaveTimer);
	GetWorldTimerManager().ClearTimer(ReturnTimer);
	ClearRound();
	SetBarrierEnabled(false);
	if (AWizardPlayerState* Player = ActivePlayer.Get())
	{
		Player->GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetHealthAttribute()).Remove(HealthHandle);
		const bool bAlive = Player->GetAttributes()->GetHealth() > 0.f;
		Player->SetInChallenge(false);
		ShowAnnouncement(FText::FromString(bRoundWon && bAlive ? TEXT("挑战成功") : (bAlive ? TEXT("挑战中止") : TEXT("挑战失败"))),
			FText::GetEmpty(),
			bRoundWon && bAlive ? FLinearColor(1.f, .8f, .28f) : FLinearColor(1.f, .3f, .28f), ReturnDelay);
	}
	HealthHandle.Reset();
	// 结果展示不限制正常操作；死亡保留零生命，传送完成后再恢复。
	GetWorldTimerManager().SetTimer(ReturnTimer, this, &ThisClass::ReturnToCamp, ReturnDelay, false);
	UE_LOG(LogTemp, Display, TEXT("挑战结束展示：%.2f秒后返回营地"), ReturnDelay);
}

void AWizardChallengeArena::ShowAnnouncement(const FText& Text, const FText& Detail, const FLinearColor& Color, float Duration)
{
	AWizardPlayerController* Controller = ActivePlayer.IsValid() ? Cast<AWizardPlayerController>(ActivePlayer->GetOwner()) : nullptr;
	if (Controller && Controller->GetUIRoot()) Controller->GetUIRoot()->ShowChallengeAnnouncementDetail(Text, Detail, Color, Duration);
}

void AWizardChallengeArena::ReturnToCamp()
{
	if (State != EWizardChallengeState::Returning) return;
	GetWorldTimerManager().ClearTimer(ReturnTimer);
	SetBarrierEnabled(false);
	if (AWizardPlayerState* Player = ActivePlayer.Get())
	{
		if (ACharacter* Avatar = Cast<ACharacter>(Player->GetAbilitySystemComponent()->GetAvatarActor()))
		{
			Avatar->GetCharacterMovement()->StopMovementImmediately();
			if (!Avatar->TeleportTo(CampReturnLocation, CampReturnRotation, false, false))
			{
				UE_LOG(LogTemp, Error, TEXT("挑战营地返回点被阻挡：%s"), *CampReturnLocation.ToString());
				Avatar->SetActorLocation(CampReturnLocation, false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
		Player->RestoreAtCamp();
	}
	HealthHandle.Reset();
	ActivePlayer.Reset();
	CurrentWave = INDEX_NONE;
	State = EWizardChallengeState::Ready;
	UE_LOG(LogTemp, Display, TEXT("挑战结束：玩家返回营地，区域已重置"));
}

void AWizardChallengeArena::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	if (ActivePlayer.IsValid())
	{
		ActivePlayer->GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetHealthAttribute()).Remove(HealthHandle);
		ActivePlayer->SetInChallenge(false);
	}
	State = EWizardChallengeState::Returning;
	ClearRound();
	Super::EndPlay(Reason);
}
