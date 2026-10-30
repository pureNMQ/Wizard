#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "UI/WizardEnemyHealthBar.h"
#include "Combat/WizardEnemy.h"
#include "GAS/WizardAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardEnemyHealthBarTest,
	"Wizard.UI.EnemyHealthBarAttributesAndLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWizardEnemyHealthBarTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Initialization = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Initialization);
	if (!TestNotNull(TEXT("血条临时世界"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); };
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWizardEnemy* First = World->SpawnActor<AWizardEnemy>(FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	AWizardEnemy* Second = World->SpawnActor<AWizardEnemy>(FVector(300.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
	if (!First || !Second) return false;
	UAbilitySystemComponent* A = First->GetAbilitySystemComponent();
	UAbilitySystemComponent* B = Second->GetAbilitySystemComponent();
	A->AddAttributeSetSubobject(First->GetAttributes());
	B->AddAttributeSetSubobject(Second->GetAttributes());
	A->InitAbilityActorInfo(First, First);
	B->InitAbilityActorInfo(Second, Second);
	UWizardEnemyHealthBar* Widget = NewObject<UWizardEnemyHealthBar>(World);
	Widget->HealthBar = NewObject<UProgressBar>(Widget);
	Widget->BindEnemy(First);
	TestEqual(TEXT("初始满血"), Widget->HealthBar->GetPercent(), 1.f);
	A->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 40.f);
	TestEqual(TEXT("伤害事件立即刷新"), Widget->HealthBar->GetPercent(), .4f);
	A->SetNumericAttributeBase(UWizardAttributeSet::GetMaxHealthAttribute(), 200.f);
	TestEqual(TEXT("上限事件立即刷新"), Widget->HealthBar->GetPercent(), .2f);
	Widget->BindEnemy(Second);
	A->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 0.f);
	TestEqual(TEXT("旧来源不再刷新血条"), Widget->HealthBar->GetPercent(), 1.f);
	B->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 0.f);
	TestEqual(TEXT("死亡归零"), Widget->HealthBar->GetPercent(), 0.f);
	TestEqual(TEXT("死亡隐藏"), Widget->GetVisibility(), ESlateVisibility::Collapsed);
	Widget->NativeDestruct();
	B->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 50.f);
	TestEqual(TEXT("析构已解除订阅"), Widget->HealthBar->GetPercent(), 0.f);
	Widget->BindEnemy(nullptr);
	TestEqual(TEXT("空来源隐藏"), Widget->GetVisibility(), ESlateVisibility::Collapsed);
	return true;
}
#endif
