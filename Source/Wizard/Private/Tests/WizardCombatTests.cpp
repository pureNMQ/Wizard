#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"
#include "AbilitySystemComponent.h"
#include "Combat/WizardEnemy.h"
#include "Combat/WizardCombatConfig.h"
#include "GAS/WizardAttributeSet.h"
#include "GAS/WizardBasicAttackAbility.h"
#include "GAS/WizardDamageExecution.h"
#include "WizardPlayerState.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/WorldSettings.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardDamageEffectTest,
	"Wizard.Combat.DamageEffectAndClamps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWizardDamageEffectTest::RunTest(const FString& Parameters)
{
	const TSubclassOf<UGameplayEffect> DamageEffectClass = LoadClass<UGameplayEffect>(nullptr, TEXT("/Game/Wizard/GAS/Effects/GE_BasicAttackDamage.GE_BasicAttackDamage_C"));
	if (!TestNotNull(TEXT("蓝图伤害 GE 可加载"), DamageEffectClass.Get())) { return false; }
	const UWorld::InitializationValues Initialization = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Initialization);
	if (!TestNotNull(TEXT("临时战斗世界"), World)) { return false; }
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	};
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWizardEnemy* Source = World->SpawnActor<AWizardEnemy>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	AWizardEnemy* Target = World->SpawnActor<AWizardEnemy>(FVector(300.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParameters);
	if (!TestNotNull(TEXT("伤害来源"), Source) || !TestNotNull(TEXT("伤害目标"), Target))
	{
		return false;
	}
	// 完整路由组件 InitializeComponent 与 Actor BeginPlay，使用和游戏相同的属性注册路径。
	World->InitializeActorsForPlay(FURL());
	World->BeginPlay();
	// 本 fixture 不创建 GameMode；正式游戏由 GameState 调用这一入口。
	World->GetWorldSettings()->NotifyBeginPlay();
	UAbilitySystemComponent* SourceASC = Source->GetAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = Target->GetAbilitySystemComponent();
	if (!TestTrue(TEXT("敌人实际执行 BeginPlay"), Source->HasActorBegunPlay() && Target->HasActorBegunPlay())
		|| !TestNotNull(TEXT("组件初始化自动注册来源属性"), SourceASC->GetSet<UWizardAttributeSet>())
		|| !TestNotNull(TEXT("组件初始化自动注册目标属性"), TargetASC->GetSet<UWizardAttributeSet>()))
	{
		return false;
	}
	SourceASC->SetNumericAttributeBase(UWizardAttributeSet::GetAttackPowerAttribute(), 30.0f);
	TargetASC->SetNumericAttributeBase(UWizardAttributeSet::GetDefenseAttribute(), 5.0f);
	FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	TestEqual(TEXT("GE 经捕获攻防扣血"), Target->GetAttributes()->GetHealth(), 75.0f);
	TestEqual(TEXT("瞬时伤害元属性被消费"), Target->GetAttributes()->GetIncomingDamage(), 0.0f);
	TestEqual(TEXT("普通攻击不消耗法力"), Source->GetAttributes()->GetMana(), 100.0f);
	TargetASC->SetNumericAttributeBase(UWizardAttributeSet::GetDefenseAttribute(), 100.0f);
	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	TestEqual(TEXT("高防御保留一点伤害"), Target->GetAttributes()->GetHealth(), 74.0f);
	TargetASC->SetNumericAttributeBase(UWizardAttributeSet::GetMaxHealthAttribute(), 50.0f);
	TestEqual(TEXT("降低上限夹取当前生命"), Target->GetAttributes()->GetHealth(), 50.0f);
	TargetASC->SetNumericAttributeBase(UWizardAttributeSet::GetMaxHealthAttribute(), 100.0f);
	TestEqual(TEXT("恢复上限不能免费治疗"), Target->GetAttributes()->GetHealth(), 50.0f);
	SourceASC->SetNumericAttributeBase(UWizardAttributeSet::GetAttackPowerAttribute(), 1000.0f);
	Spec = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, SourceASC->MakeEffectContext());
	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	TestEqual(TEXT("致命伤害夹取到零"), Target->GetAttributes()->GetHealth(), 0.0f);
	TestEqual(TEXT("零攻击力不产生保底伤害"), WizardCombat::CalculateDamage(0.0f, 100.0f), 0.0f);
	AWizardPlayerState* InvalidState = World->SpawnActor<AWizardPlayerState>();
	if (!TestNotNull(TEXT("缺配置失败用例 PlayerState"), InvalidState)) { return false; }
	UWizardCombatConfig* MissingAbility = NewObject<UWizardCombatConfig>(InvalidState);
	TestFalse(TEXT("缺少普通攻击类的配置被拒绝"), MissingAbility->IsConfigurationValid());
	MissingAbility->BasicAttackAbilityClass = UWizardBasicAttackAbility::StaticClass();
	TestFalse(TEXT("缺少伤害与冷却 GE 的能力被拒绝"), MissingAbility->IsConfigurationValid());
	InvalidState->CombatConfig = MissingAbility;
	InvalidState->InitializeAvatar(Source);
	TestEqual(TEXT("缺配置时不授予不完整能力"), InvalidState->GetAbilitySystemComponent()->GetActivatableAbilities().Num(), 0);
	return true;
}

#endif
