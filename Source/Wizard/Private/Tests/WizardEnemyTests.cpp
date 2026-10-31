#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Combat/WizardEnemy.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Combat/WizardEnemyAttackAbility.h"
#include "Combat/WizardEnemyAI.h"
#include "WizardCharacter.h"
#include "WizardPlayerState.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "GameFramework/PlayerController.h"
#include "Navigation/PathFollowingComponent.h"
#include "GAS/WizardAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardEnemyDamageTest, "Wizard.Enemy.DamageShieldDeathAndInvalidTarget", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardEnemyDamageTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Initialization = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Initialization);
	if (!TestNotNull(TEXT("临时世界"), World)) { return false; }
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWizardEnemy* Enemy = World->SpawnActor<AWizardEnemy>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	AWizardEnemy* Target = World->SpawnActor<AWizardEnemy>(FVector(500,0,0), FRotator::ZeroRotator, Spawn);
	if (!Enemy || !Target) { return false; }
	// 无 BT 的已有靶子不能变成攻击玩家的活动怪物。
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	World->GetWorldSettings()->NotifyBeginPlay();
	UAbilitySystemComponent* SourceASC = Enemy->GetAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
	TestFalse(TEXT("空目标拒绝"), Enemy->RequestAttack(nullptr));
	TestFalse(TEXT("怪物不能攻击同类"), Enemy->RequestAttack(Target));
	TestEqual(TEXT("未配置靶子不授予攻击能力"), SourceASC->GetActivatableAbilities().Num(), 0);
	SourceASC->SetNumericAttributeBase(UWizardAttributeSet::GetAttackPowerAttribute(), 30);
	TargetASC->SetNumericAttributeBase(UWizardAttributeSet::GetDefenseAttribute(), 5);
	TargetASC->SetNumericAttributeBase(UWizardAttributeSet::GetShieldAttribute(), 10);
	FGameplayEffectSpecHandle Damage = SourceASC->MakeOutgoingSpec(UWizardEnemyDamageEffect::StaticClass(), 1, SourceASC->MakeEffectContext());
	SourceASC->ApplyGameplayEffectSpecToTarget(*Damage.Data.Get(), TargetASC);
	TestEqual(TEXT("敌伤害复用护盾吸收"), Target->GetAttributes()->GetShield(), 0.f);
	TestEqual(TEXT("护盾后的伤害经攻防规则扣血"), Target->GetAttributes()->GetHealth(), 85.f);
	SourceASC->SetNumericAttributeBase(UWizardAttributeSet::GetAttackPowerAttribute(), 1000);
	Damage = SourceASC->MakeOutgoingSpec(UWizardEnemyDamageEffect::StaticClass(), 1, SourceASC->MakeEffectContext());
	SourceASC->ApplyGameplayEffectSpecToTarget(*Damage.Data.Get(), TargetASC);
	TestTrue(TEXT("致命伤害进入死亡态"), Target->IsDead());
	TestFalse(TEXT("死亡怪物拒绝攻击"), Target->RequestAttack(Enemy));
	SourceASC->ApplyGameplayEffectSpecToTarget(*Damage.Data.Get(), TargetASC);
	TestEqual(TEXT("重复致命伤害生命保持零"), Target->GetAttributes()->GetHealth(), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardEnemyPursuitRangeTest, "Wizard.Enemy.PursuitDoesNotArriveOutsideAttackRange", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardEnemyPursuitRangeTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	if (!TestNotNull(TEXT("追击回归世界"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWizardEnemy* Enemy = World->SpawnActor<AWizardEnemy>(FVector(0,0,100), FRotator::ZeroRotator, Spawn);
	AWizardCharacter* Target = World->SpawnActor<AWizardCharacter>(FVector(180,0,100), FRotator::ZeroRotator, Spawn);
	AWizardPlayerState* Player = World->SpawnActor<AWizardPlayerState>();
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	if (!Enemy || !Target || !Player || !Controller) return false;
	Enemy->BehaviorTree = LoadObject<UBehaviorTree>(nullptr, TEXT("/Game/Wizard/AI/BT_Enemy.BT_Enemy"));
	Enemy->AttackRange = 160.f;
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	World->GetWorldSettings()->NotifyBeginPlay();
	Controller->Possess(Target);
	Target->SetPlayerState(Player);
	Player->InitializeAvatar(Target);
	AWizardEnemyAIController* AI = Cast<AWizardEnemyAIController>(Enemy->GetController());
	if (!TestNotNull(TEXT("真实敌人控制器"), AI)) return false;
	UBehaviorTreeComponent* Tree = NewObject<UBehaviorTreeComponent>(AI);
	Tree->RegisterComponent();
	UWizardBTTask_PursueTarget* Task = NewObject<UWizardBTTask_PursueTarget>(Tree);
	int32 FalseArrivals = 0;
	const FDelegateHandle Handle = AI->GetPathFollowingComponent()->OnRequestFinished.AddLambda(
		[&FalseArrivals](FAIRequestID, const FPathFollowingResult& Result)
		{
			if (Result.IsSuccess()) ++FalseArrivals;
		});
	ON_SCOPE_EXIT { AI->GetPathFollowingComponent()->OnRequestFinished.Remove(Handle); };
	// 不依赖导航数据：只检查真实追击任务是否错误地宣布到达，路径构建失败不当作已到达。
	for (float Distance : { 161.f, 180.f, 190.f })
	{
		Target->SetActorLocation(FVector(Distance,0,100));
		TestFalse(TEXT("测试目标确实在攻击范围外"), Enemy->CanAttackTarget(Target));
		Task->TickTask(*Tree, nullptr, .016f);
		TestEqual(*FString::Printf(TEXT("中心距%.0f时不能错误停止追击"), Distance), FalseArrivals, 0);
	}
	Target->SetActorLocation(FVector(150,0,100));
	TestTrue(TEXT("同一目标进入160以内可攻击，排除目标或视线无效"), Enemy->CanAttackTarget(Target));
	return true;
}
#endif
