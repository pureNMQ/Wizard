// Wizard 项目：结果展示不额外限制玩法，完整停留后才返回营地；死亡恢复遵循既有规则。
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Challenge/WizardChallengeArena.h"
#include "Challenge/WizardBarrierVisualComponent.h"
#include "Combat/WizardEnemy.h"
#include "WizardCharacter.h"
#include "WizardPlayerState.h"
#include "GAS/WizardAttributeSet.h"
#include "Skills/WizardSkillLoadoutComponent.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardChallengeResultDelayTest,
	"Wizard.Challenge.ResultDelayAndRepeatLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWizardChallengeResultDelayTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("结果生命周期测试世界"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWizardChallengeArena* Arena = World->SpawnActor<AWizardChallengeArena>();
	AWizardPlayerState* Player = World->SpawnActor<AWizardPlayerState>();
	AWizardCharacter* Avatar = World->SpawnActor<AWizardCharacter>(FVector(0,0,100), FRotator::ZeroRotator, Params);
	if (!Arena || !Player || !Avatar) return false;
	Arena->CampReturnLocation = FVector(5000,0,100);
	Arena->SpawnOffsets.Add(FVector(500,0,100));
	FWizardChallengeWave Wave;
	FWizardChallengeSpawn Spawn;
	Spawn.EnemyClass = AWizardEnemy::StaticClass();
	Wave.Spawns.Add(Spawn);
	Arena->Waves.Add(Wave);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	World->GetWorldSettings()->NotifyBeginPlay();
	Player->InitializeAvatar(Avatar);
	Avatar->GetCharacterMovement()->DisableMovement();
	const FWizardLoadoutSnapshot Loadout = Player->GetSkills()->GetSnapshot();
	auto AdvanceFrames = [World](int32 Count)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			++GFrameCounter;
			World->Tick(LEVELTICK_All, .05f);
		}
	};
	auto HasBlockingBarrier = [Arena]()
	{
		TArray<UBoxComponent*> Boxes;
		Arena->GetComponents(Boxes);
		return Boxes.ContainsByPredicate([](UBoxComponent* Box) { return Box->GetCollisionEnabled() != ECollisionEnabled::NoCollision; });
	};
	TestEqual(TEXT("默认结果停留为2秒"), Arena->ReturnDelay, 2.f);
	for (int32 Round = 0; Round < 2; ++Round)
	{
		const bool bDeath = Round == 1;
		Avatar->SetActorLocation(FVector(0,0,100));
		if (!TestTrue(TEXT("可重复开始挑战"), Arena->TryStartChallenge(Player))) return false;
		TestTrue(TEXT("战斗期间结界视觉启用"), Arena->BarrierVisual->IsVisible());
		TestTrue(TEXT("战斗期间空气墙阻挡"), HasBlockingBarrier());
		AWizardEnemy* Enemy = nullptr;
		for (TActorIterator<AWizardEnemy> It(World); It; ++It) if (It->SourceChallenge.Get() == Arena) { Enemy = *It; break; }
		if (!TestNotNull(TEXT("真实波次敌人"), Enemy)) return false;
		if (!bDeath) Player->GetSkills()->ApplyShield(40.f, 10.f);
		if (bDeath) Player->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 0.f);
		else Enemy->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 0.f);
		// NextTick在TimerManager本帧推进前后排入时可能跨两个帧边界；只给有限帧数，不放宽结果停留时长。
		for (int32 Frame = 0; Frame < 3 && Arena->State != EWizardChallengeState::Returning; ++Frame) AdvanceFrames(1);
		AddInfo(FString::Printf(TEXT("轮次%d：结束请求后State=%d HP=%.1f WorldTime=%.3f Location=%s"), Round + 1,
			static_cast<int32>(Arena->State), Player->GetAttributes()->GetHealth(), World->GetTimeSeconds(), *Avatar->GetActorLocation().ToString()));
		if (!TestEqual(TEXT("胜败进入结果展示"), Arena->State, EWizardChallengeState::Returning)) return false;
		const double ResultStart = World->GetTimeSeconds();
		TestTrue(TEXT("展示期间禁止重新开始"), !Arena->TryStartChallenge(Player));
		TestFalse(TEXT("展示期间已解除挑战状态"), Player->IsInChallenge());
		TestEqual(TEXT("展示时已经清空活怪"), Arena->GetLivingEnemyCount(), 0);
		TestFalse(TEXT("展示期间结界已隐藏"), Arena->BarrierVisual->IsVisible());
		TestFalse(TEXT("展示期间空气墙已开放"), HasBlockingBarrier());
		// 结果阶段允许自由移动；只排除提前回营地，不把移动或无物理场景中的下落判为提前传送。
		TestTrue(TEXT("结束时尚未回营地"), FVector::Dist2D(Avatar->GetActorLocation(), Arena->CampReturnLocation) > 150.f);
		FVector SkillTarget;
		const UWizardSkillDefinition* ProjectileSkill = nullptr;
		for (const UWizardSkillDefinition* Definition : Player->GetSkills()->Catalog->Skills)
		{
			if (Definition && Definition->Behavior == EWizardSkillBehavior::Projectile) { ProjectileSkill = Definition; break; }
		}
		if (!TestNotNull(TEXT("选取不依赖地面的正常投射物技能"), ProjectileSkill)) return false;
		// 此fixture没有动画实例；检查真实技能业务入口的可释放条件，蒙太奇及按键执行在PIE另验。
		TestEqual(TEXT("结果展示保留正常技能条件，死亡仅受原生限制"),
			Player->GetSkills()->CheckSkill(ProjectileSkill, Avatar->GetActorLocation() + FVector(600,0,0), SkillTarget),
			bDeath ? EWizardSkillResult::Dead : EWizardSkillResult::Success);
		if (bDeath) TestEqual(TEXT("死亡展示时尚未恢复生命"), Player->GetAttributes()->GetHealth(), 0.f);
		else TestEqual(TEXT("胜利展示保留既有护盾效果"), Player->GetAttributes()->GetShield(), 40.f);
		// 模拟迟到的重复死亡通知，不能延长或重新安排结果计时。
		Player->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), bDeath ? 0.f : Player->GetAttributes()->GetHealth());
		AdvanceFrames(37);
		TestEqual(TEXT("1.85秒时仍展示结果"), Arena->State, EWizardChallengeState::Returning);
		if (bDeath) TestEqual(TEXT("死亡完整停留期间保持零生命"), Player->GetAttributes()->GetHealth(), 0.f);
		TestTrue(TEXT("倒计时结束前尚未回营地"), FVector::Dist2D(Avatar->GetActorLocation(), Arena->CampReturnLocation) > 150.f);
		AdvanceFrames(4);
		AddInfo(FString::Printf(TEXT("轮次%d：结果等待%.3f秒后State=%d HP=%.1f Location=%s Camp=%s"), Round + 1,
			World->GetTimeSeconds() - ResultStart, static_cast<int32>(Arena->State), Player->GetAttributes()->GetHealth(),
			*Avatar->GetActorLocation().ToString(), *Arena->CampReturnLocation.ToString()));
		TestTrue(TEXT("结果展示计时至少2秒"), World->GetTimeSeconds() - ResultStart >= 2.0);
		TestEqual(TEXT("完整2秒后恢复就绪"), Arena->State, EWizardChallengeState::Ready);
		// TeleportTo允许胶囊与平面约束校正高度；无物理场景fixture只严格约束水平目的地，实际地图另验落地。
		TestTrue(TEXT("完整停留后回到营地水平位置"), FVector::Dist2D(Avatar->GetActorLocation(), Arena->CampReturnLocation) <= .1f);
		TestTrue(TEXT("返回高度在角色落地校正范围"), FMath::Abs(Avatar->GetActorLocation().Z - Arena->CampReturnLocation.Z) <= 150.f);
		TestFalse(TEXT("返回后解除战斗状态"), Player->IsInChallenge());
		TestFalse(TEXT("返回后隐藏结界"), Arena->BarrierVisual->IsVisible());
		TestFalse(TEXT("返回后开放空气墙"), HasBlockingBarrier());
		TestEqual(TEXT("返回后恢复生命"), Player->GetAttributes()->GetHealth(), Player->GetAttributes()->GetMaxHealth());
		TestTrue(TEXT("结果展示不修改搭配"), Loadout.Slots == Player->GetSkills()->GetSnapshot().Slots);
	}
	return true;
}
#endif
