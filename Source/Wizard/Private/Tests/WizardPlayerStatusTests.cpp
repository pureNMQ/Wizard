#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "UI/WizardPlayerStatusWidget.h"
#include "WizardPlayerState.h"
#include "GAS/WizardAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardPlayerStatusTest,
	"Wizard.UI.PlayerStatusAttributesAndLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWizardPlayerStatusTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues Initialization = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Initialization);
	if (!TestNotNull(TEXT("临时世界"), World)) return false;
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
	};
	AWizardPlayerState* First = World->SpawnActor<AWizardPlayerState>();
	AWizardPlayerState* Second = World->SpawnActor<AWizardPlayerState>();
	if (!TestNotNull(TEXT("第一来源"), First) || !TestNotNull(TEXT("第二来源"), Second)) return false;
	UAbilitySystemComponent* A = First->GetAbilitySystemComponent();
	UAbilitySystemComponent* B = Second->GetAbilitySystemComponent();
	// 注册真实属性集，不启动战斗或修改上限业务约束。
	A->AddAttributeSetSubobject(First->GetAttributes());
	B->AddAttributeSetSubobject(Second->GetAttributes());
	A->InitAbilityActorInfo(First, First);
	B->InitAbilityActorInfo(Second, Second);
	UWizardPlayerStatusWidget* Widget = NewObject<UWizardPlayerStatusWidget>(World);
	Widget->HealthBar = NewObject<UProgressBar>(Widget);
	Widget->ManaBar = NewObject<UProgressBar>(Widget);
	Widget->HealthText = NewObject<UTextBlock>(Widget);
	Widget->ManaText = NewObject<UTextBlock>(Widget);
	Widget->GoldText = NewObject<UTextBlock>(Widget);
	auto Check = [this, Widget](float HealthRatio, float ManaRatio, const TCHAR* Health, const TCHAR* Mana)
	{
		TestEqual(TEXT("实际生命条"), Widget->HealthBar->GetPercent(), HealthRatio);
		TestEqual(TEXT("实际法力条"), Widget->ManaBar->GetPercent(), ManaRatio);
		TestEqual(TEXT("实际生命文本无空格"), Widget->HealthText->GetText().ToString(), FString(Health));
		TestEqual(TEXT("实际法力文本无空格"), Widget->ManaText->GetText().ToString(), FString(Mana));
	};
	Widget->BindPlayer(nullptr);
	Check(0.f, 0.f, TEXT("0/0"), TEXT("0/0"));
	TestEqual(TEXT("未绑定金币为零"), Widget->GoldText->GetText().ToString(), FString(TEXT("0")));
	First->TryAddGold(12);
	Widget->BindPlayer(First);
	TestEqual(TEXT("绑定刷新当前金币"), Widget->GoldText->GetText().ToString(), FText::AsNumber(12).ToString());
	First->TryAddGold(8);
	TestEqual(TEXT("金币事件刷新"), Widget->GoldText->GetText().ToString(), FText::AsNumber(20).ToString());
	Check(1.f, 1.f, TEXT("100/100"), TEXT("100/100"));
	A->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 50.f);
	Check(.5f, 1.f, TEXT("50/100"), TEXT("100/100"));
	A->SetNumericAttributeBase(UWizardAttributeSet::GetMaxHealthAttribute(), 200.f);
	Check(.25f, 1.f, TEXT("50/200"), TEXT("100/100"));
	A->SetNumericAttributeBase(UWizardAttributeSet::GetManaAttribute(), 25.f);
	Check(.25f, .25f, TEXT("50/200"), TEXT("25/100"));
	A->SetNumericAttributeBase(UWizardAttributeSet::GetMaxManaAttribute(), 50.f);
	Check(.25f, .5f, TEXT("50/200"), TEXT("25/50"));
	Widget->BindPlayer(First);
	Widget->BindPlayer(Second);
	First->TryAddGold(1);
	TestEqual(TEXT("切换来源解绑旧金币事件"), Widget->GoldText->GetText().ToString(), FText::AsNumber(0).ToString());
	Second->TryAddGold(7);
	TestEqual(TEXT("新来源金币刷新"), Widget->GoldText->GetText().ToString(), FText::AsNumber(7).ToString());
	A->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 20.f);
	A->SetNumericAttributeBase(UWizardAttributeSet::GetMaxHealthAttribute(), 80.f);
	A->SetNumericAttributeBase(UWizardAttributeSet::GetManaAttribute(), 10.f);
	A->SetNumericAttributeBase(UWizardAttributeSet::GetMaxManaAttribute(), 40.f);
	Check(1.f, 1.f, TEXT("100/100"), TEXT("100/100"));
	B->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 30.f);
	B->SetNumericAttributeBase(UWizardAttributeSet::GetManaAttribute(), 40.f);
	Check(.3f, .4f, TEXT("30/100"), TEXT("40/100"));
	Widget->NativeDestruct();
	Second->TryAddGold(2);
	TestEqual(TEXT("销毁后金币不再刷新"), Widget->GoldText->GetText().ToString(), FText::AsNumber(7).ToString());
	B->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 10.f);
	B->SetNumericAttributeBase(UWizardAttributeSet::GetMaxHealthAttribute(), 50.f);
	B->SetNumericAttributeBase(UWizardAttributeSet::GetManaAttribute(), 20.f);
	B->SetNumericAttributeBase(UWizardAttributeSet::GetMaxManaAttribute(), 50.f);
	Check(.3f, .4f, TEXT("30/100"), TEXT("40/100"));
	// 无 OwningPlayer 的重建必须清空旧来源；零上限防御通过未绑定态验证。
	Widget->NativeConstruct();
	Check(0.f, 0.f, TEXT("0/0"), TEXT("0/0"));
	Widget->BindPlayer(Second);
	Check(.2f, .4f, TEXT("10/50"), TEXT("20/50"));
	Widget->NativeDestruct();
	return true;
}
#endif
