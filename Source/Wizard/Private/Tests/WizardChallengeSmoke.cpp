// Wizard 项目：真实地图挑战生命周期复跑命令；会操作当前开发世界中的玩家和挑战。
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST
#include "Challenge/WizardChallengeArena.h"
#include "Combat/WizardEnemy.h"
#include "WizardPlayerState.h"
#include "Skills/WizardSkillLoadoutComponent.h"
#include "GAS/WizardAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Containers/Ticker.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogWizardChallengeSmoke, Log, All);
namespace WizardChallengeSmoke
{
	struct FRun : TSharedFromThis<FRun>
	{
		TWeakObjectPtr<UWorld> World;
		TWeakObjectPtr<AWizardChallengeArena> Arena;
		TWeakObjectPtr<AWizardPlayerState> Player;
		FWizardLoadoutSnapshot Snapshot;
		int32 Phase = 0, SeenWaves = 0;
		double Start = 0;
		double EndRequestedAt = -1.;
		bool bObservedResult = false;
		void Finish(bool bPassed, const FString& Reason)
		{
			UE_LOG(LogWizardChallengeSmoke, Display, TEXT("WIZARD_CHALLENGE_SMOKE_%s: %s"), bPassed ? TEXT("PASS") : TEXT("FAIL"), *Reason);
		}
		bool Tick(float)
		{
			if (!Arena.IsValid() || !Player.IsValid() || !World.IsValid()) { Finish(false, TEXT("世界或对象失效")); return false; }
			if (FPlatformTime::Seconds() - Start > 60.) { Finish(false, TEXT("60秒超时")); return false; }
			AWizardChallengeArena* Area = Arena.Get();
			AWizardPlayerState* State = Player.Get();
			AActor* Avatar = State->GetAbilitySystemComponent()->GetAvatarActor();
			if (!Avatar) { Finish(false, TEXT("玩家Avatar失效")); return false; }
			if (Phase == 0 || Phase == 3)
			{
				if (Area->State != EWizardChallengeState::Ready) { Finish(false, TEXT("挑战未就绪")); return false; }
				State->RestoreAtCamp();
				EndRequestedAt = -1.;
				bObservedResult = false;
				Avatar->SetActorLocation(Area->GetActorLocation() + FVector(0,0,100), false, nullptr, ETeleportType::TeleportPhysics);
				// 只移动玩家，由区域真实监测入口启动，不能用反射直改状态代替入圈。
				++Phase;
				return true;
			}
			if (Phase == 1 && Area->State == EWizardChallengeState::Fighting)
			{
				FHitResult Hit;
				FCollisionQueryParams Query(SCENE_QUERY_STAT(ChallengeWallSmoke), false, Avatar);
				for (TActorIterator<AWizardEnemy> It(World.Get()); It; ++It) Query.AddIgnoredActor(*It);
				const FVector From = Area->GetActorLocation() + FVector(0,0,600);
				const bool bBlocked = World->SweepSingleByChannel(Hit, From, From + FVector(Area->ArenaRadius + 200.f,0,0), FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(42.f), Query);
				if (!bBlocked || Hit.GetActor() != Area) { Finish(false, TEXT("空气墙未阻挡Pawn Sweep")); return false; }
				UE_LOG(LogWizardChallengeSmoke, Display, TEXT("入圈触发与空气墙Sweep通过"));
				Phase = 2;
			}
			if (Phase == 2 && Area->State == EWizardChallengeState::Fighting)
			{
				SeenWaves = FMath::Max(SeenWaves, Area->CurrentWave + 1);
				TArray<AWizardEnemy*> Targets;
				for (TActorIterator<AWizardEnemy> It(World.Get()); It; ++It) if (It->SourceChallenge.Get() == Area && !It->IsDead()) Targets.Add(*It);
				if (Area->CurrentWave + 1 == Area->Waves.Num() && !Targets.IsEmpty())
				{
					EndRequestedAt = World->GetTimeSeconds();
				}
				for (AWizardEnemy* Enemy : Targets) Enemy->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 0.f);
			}
			if ((Phase == 2 || Phase == 5) && Area->State == EWizardChallengeState::Returning)
			{
				bObservedResult = true;
				// 结果阶段允许移动；只有抵达营地才算提前传送，高度变化不影响判断。
				if (State->IsInChallenge() || FVector::Dist2D(Avatar->GetActorLocation(), Area->CampReturnLocation) <= 150.f
					|| (Phase == 5 && State->GetAttributes()->GetHealth() != 0.f))
				{
					Finish(false, TEXT("结果展示未解除挑战状态、提前回营地或死亡提前恢复")); return false;
				}
			}
			if ((Phase == 2 || Phase == 5) && Area->State == EWizardChallengeState::Ready)
			{
				const double ResultSeconds = World->GetTimeSeconds() - EndRequestedAt;
				if (!bObservedResult || EndRequestedAt < 0. || ResultSeconds + .02 < Area->ReturnDelay)
				{
					Finish(false, TEXT("未完整展示2秒即回营地")); return false;
				}
				UE_LOG(LogWizardChallengeSmoke, Display, TEXT("结果展示延迟通过：世界时间%.2f秒，配置%.2f秒"), ResultSeconds, Area->ReturnDelay);
				bool bLeaked = false;
				for (TActorIterator<AWizardEnemy> It(World.Get()); It; ++It) if (It->SourceChallenge.Get() == Area) bLeaked = true;
				if (bLeaked || State->IsInChallenge() || State->GetAttributes()->GetHealth() != State->GetAttributes()->GetMaxHealth()
					|| State->GetAttributes()->GetMana() != State->GetAttributes()->GetMaxMana()
					|| FVector::Dist(Avatar->GetActorLocation(), Area->CampReturnLocation) > 150.f
					|| State->GetSkills()->GetSnapshot().Slots != Snapshot.Slots)
				{
					Finish(false, TEXT("返回营地位置/恢复/搭配/残留检查失败")); return false;
				}
				if (Phase == 5) { Finish(true, TEXT("真实地图入圈、空气墙、全部波次胜利返回、重复入圈死亡返回及清理通过；未覆盖物理输入、美术与成品包")); return false; }
				if (SeenWaves != Area->Waves.Num()) { Finish(false, TEXT("未观察全部波次")); return false; }
				UE_LOG(LogWizardChallengeSmoke, Display, TEXT("全部%d波胜利回营地通过"), SeenWaves);
				Phase = 3;
			}
			if (Phase == 4 && Area->State == EWizardChallengeState::Fighting)
			{
				EndRequestedAt = World->GetTimeSeconds();
				State->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 0.f);
				Phase = 5;
			}
			return true;
		}
	};
	static FAutoConsoleCommandWithWorld Command(TEXT("Wizard.ChallengeSmokeTest"), TEXT("验证当前开发地图的挑战闭环；玩家最终留在营地"), FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* World)
	{
		if (!World || !World->IsGameWorld() || World->IsPaused()) return;
		AWizardChallengeArena* Arena = nullptr;
		for (TActorIterator<AWizardChallengeArena> It(World); It; ++It) { Arena = *It; break; }
		APawn* Pawn = UGameplayStatics::GetPlayerPawn(World, 0);
		AWizardPlayerState* Player = Pawn ? Pawn->GetPlayerState<AWizardPlayerState>() : nullptr;
		if (!Arena || !Player) { UE_LOG(LogWizardChallengeSmoke, Error, TEXT("WIZARD_CHALLENGE_SMOKE_FAIL: 缺少区域或玩家")); return; }
		TSharedRef<FRun> Run = MakeShared<FRun>();
		Run->World = World;
		Run->Arena = Arena;
		Run->Player = Player;
		Run->Snapshot = Player->GetSkills()->GetSnapshot();
		Run->Start = FPlatformTime::Seconds();
		FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Run](float Delta) { return Run->Tick(Delta); }), .1f);
	}));
}
#endif
