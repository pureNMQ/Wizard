#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Challenge/WizardChallengeArena.h"
#include "Combat/WizardEnemy.h"
#include "Combat/WizardEnemyProjectile.h"
#include "WizardPlayerState.h"
#include "GAS/WizardAttributeSet.h"
#include "Skills/WizardSkillLoadoutComponent.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "Misc/ScopeExit.h"
#include "UObject/GarbageCollection.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardChallengeConfigurationTest, "Wizard.Challenge.ConfigurationAndCampRestore", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardChallengeConfigurationTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("测试世界"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	AWizardChallengeArena* Arena = World->SpawnActor<AWizardChallengeArena>();
	TestFalse(TEXT("缺波次与出生点配置被拒绝"), Arena->IsConfigurationValid());
	Arena->CampReturnLocation = FVector(5000,0,100);
	Arena->SpawnOffsets.Add(FVector(500,0,100));
	FWizardChallengeWave Wave;
	FWizardChallengeSpawn Spawn;
	Spawn.EnemyClass = LoadClass<AWizardEnemy>(nullptr, TEXT("/Game/Wizard/Enemies/BP_Slime.BP_Slime_C"));
	Wave.Spawns.Add(Spawn);
	Arena->Waves.Add(Wave);
	TestTrue(TEXT("合法简单波次"), Arena->IsConfigurationValid());
	Arena->Waves[0].Spawns[0].EnemyClass = AWizardEnemy::StaticClass();
	AddExpectedError(TEXT("波次敌人配置无效"), EAutomationExpectedErrorFlags::Contains, 1);
	TestFalse(TEXT("无行为树的靶子不能用作挑战敌人"), Arena->IsConfigurationValid());
	Arena->Waves[0].Spawns[0].EnemyClass = Spawn.EnemyClass;
	Arena->Waves[0].Spawns[0].Count = 0;
	TestFalse(TEXT("空生成不能锁住玩家"), Arena->IsConfigurationValid());
	Arena->Waves[0].Spawns[0].Count = 1;
	Arena->CampReturnLocation = FVector::ZeroVector;
	TestFalse(TEXT("返回点在战斗区内被拒绝"), Arena->IsConfigurationValid());
	Arena->CampReturnLocation = FVector(5000,0,100);
	AWizardPlayerState* Player = World->SpawnActor<AWizardPlayerState>();
	APawn* Avatar = World->SpawnActor<ACharacter>(FVector(5000,0,100), FRotator::ZeroRotator);
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	World->GetWorldSettings()->NotifyBeginPlay();
	Player->InitializeAvatar(Avatar);
	const FWizardLoadoutSnapshot Before = Player->GetSkills()->GetSnapshot();
	Player->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 0.f);
	Player->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetManaAttribute(), 2.f);
	Player->SetInChallenge(true);
	Player->RestoreAtCamp();
	TestEqual(TEXT("死亡回营地满血"), Player->GetAttributes()->GetHealth(), Player->GetAttributes()->GetMaxHealth());
	TestEqual(TEXT("回营地满法力"), Player->GetAttributes()->GetMana(), Player->GetAttributes()->GetMaxMana());
	TestFalse(TEXT("解除战斗状态"), Player->IsInChallenge());
	TestTrue(TEXT("技能搭配保留"), Player->GetSkills()->GetSnapshot().Slots == Before.Slots);
	TestFalse(TEXT("营地外不可触发挑战"), Arena->TryStartChallenge(Player));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardChallengeProjectileOwnershipTest, "Wizard.Challenge.ProjectileCleanupAfterSourceDestroyed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardChallengeProjectileOwnershipTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("清理回归世界"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	auto CreateArena = [World](const FVector& Location)
	{
		AWizardChallengeArena* Arena = World->SpawnActor<AWizardChallengeArena>(Location, FRotator::ZeroRotator);
		Arena->CampReturnLocation = Location + FVector(5000,0,100);
		Arena->SpawnOffsets.Add(FVector(500,0,100));
		FWizardChallengeWave Wave;
		FWizardChallengeSpawn Spawn;
		Spawn.EnemyClass = LoadClass<AWizardEnemy>(nullptr, TEXT("/Game/Wizard/Enemies/BP_Slime.BP_Slime_C"));
		Wave.Spawns.Add(Spawn);
		Arena->Waves.Add(Wave);
		return Arena;
	};
	AWizardChallengeArena* Arena = CreateArena(FVector::ZeroVector);
	AWizardChallengeArena* OtherArena = CreateArena(FVector(10000,0,0));
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Arena;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWizardEnemy* Source = World->SpawnActor<AWizardEnemy>(FVector(500,0,100), FRotator::ZeroRotator, SpawnParams);
	Source->SourceChallenge = Arena;
	SpawnParams.Owner = OtherArena;
	AWizardEnemy* OtherSource = World->SpawnActor<AWizardEnemy>(FVector(10500,0,100), FRotator::ZeroRotator, SpawnParams);
	OtherSource->SourceChallenge = OtherArena;
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	World->GetWorldSettings()->NotifyBeginPlay();
	// 重现真实Pawn占有将Owner改写为AIController，归属不能从Owner链推导。
	AAIController* Controller = World->SpawnActor<AAIController>();
	Controller->Possess(Source);
	TestTrue(TEXT("占有已覆盖Enemy Owner"), Source->GetOwner() == Controller);
	AWizardEnemyProjectile* Projectile = World->SpawnActor<AWizardEnemyProjectile>();
	Projectile->SetOwner(Source);
	Projectile->Initialize(Source->GetAbilitySystemComponent(), FGameplayEffectSpecHandle(), 100.f);
	AWizardEnemyProjectile* OtherProjectile = World->SpawnActor<AWizardEnemyProjectile>();
	OtherProjectile->SetOwner(OtherSource);
	OtherProjectile->Initialize(OtherSource->GetAbilitySystemComponent(), FGameplayEffectSpecHandle(), 100.f);
	AWizardEnemyProjectile* UnownedProjectile = World->SpawnActor<AWizardEnemyProjectile>();
	AActor* UnrelatedActor = World->SpawnActor<AActor>();
	Source->Destroy();
	CollectGarbage(RF_NoFlags);
	TestTrue(TEXT("发射者销毁GC后仍属于原挑战"), Projectile->BelongsToChallenge(Arena));
	TestFalse(TEXT("不匹配空挑战"), UnownedProjectile->BelongsToChallenge(nullptr));
	TestFalse(TEXT("其他挑战弹丸归属隔离"), OtherProjectile->BelongsToChallenge(Arena));
	Arena->Destroy();
	TestTrue(TEXT("原挑战弹丸被清理"), Projectile->IsActorBeingDestroyed());
	TestTrue(TEXT("其他挑战弹丸保留"), IsValid(OtherProjectile) && !OtherProjectile->IsActorBeingDestroyed());
	TestTrue(TEXT("无归属弹丸保留"), IsValid(UnownedProjectile) && !UnownedProjectile->IsActorBeingDestroyed());
	TestTrue(TEXT("无Owner场景Actor保留"), IsValid(UnrelatedActor) && !UnrelatedActor->IsActorBeingDestroyed());
	return true;
}
#endif
