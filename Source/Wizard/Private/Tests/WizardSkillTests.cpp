// Wizard 项目：真实ASC/GE及公开搭配接口的失败原子性与生命周期验证。
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "WizardPlayerState.h"
#include "WizardCharacter.h"
#include "Combat/WizardEnemy.h"
#include "Skills/WizardSkillLoadoutComponent.h"
#include "Skills/WizardSkillEffects.h"
#include "Skills/WizardAlchemyStation.h"
#include "Interaction/WizardInteractionComponent.h"
#include "Interaction/WizardInteractionTarget.h"
#include "GAS/WizardAttributeSet.h"
#include "UI/WizardSkillMenuController.h"

namespace WizardSkillTests
{
	struct FFixture
	{
		UWorld* World;
		AWizardPlayerState* State;
		AWizardCharacter* Avatar;
		AWizardEnemy* Enemy;
		FFixture(bool bPhysics = false)
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(bPhysics).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
			World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
			State = World->SpawnActor<AWizardPlayerState>();
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Avatar = World->SpawnActor<AWizardCharacter>(AWizardCharacter::StaticClass(), FVector(0,0,100), FRotator::ZeroRotator, Params);
			Enemy = World->SpawnActor<AWizardEnemy>(AWizardEnemy::StaticClass(), FVector(0,1000,100), FRotator::ZeroRotator, Params);
			World->InitializeActorsForPlay(FURL());
			World->BeginPlay();
			World->GetWorldSettings()->NotifyBeginPlay();
			State->InitializeAvatar(Avatar);
		}
		~FFixture() { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); }
		void Advance(float Seconds)
		{
			// TimerManager每帧仅推进一次；独立临时世界也按引擎帧边界推进，避免把同帧重入当作到期。
			for (float Elapsed = 0.f; Elapsed < Seconds; Elapsed += .05f)
			{
				++GFrameCounter;
				World->Tick(LEVELTICK_All, .05f);
			}
		}
	};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardSkillLoadoutTest, "Wizard.Skills.DefinitionsAndLoadout", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardSkillLoadoutTest::RunTest(const FString& Parameters)
{
	WizardSkillTests::FFixture Fixture;
	UWizardSkillLoadoutComponent* Skills = Fixture.State->GetSkills();
	if (!TestNotNull(TEXT("技能目录可加载"), Skills->Catalog.Get())) return false;
	TestTrue(TEXT("六技能配置完整且ID/冷却独立"), Skills->Catalog->IsConfigurationValid());
	TestEqual(TEXT("默认四槽"), Skills->GetSnapshot().Slots.Num(), 4);
	TestEqual(TEXT("六技能加普攻只授予一次"), Fixture.State->GetAbilitySystemComponent()->GetActivatableAbilities().Num(), 7);
	Fixture.State->InitializeAvatar(Fixture.Avatar);
	TestEqual(TEXT("重复初始化无重复授予"), Fixture.State->GetAbilitySystemComponent()->GetActivatableAbilities().Num(), 7);
	const FWizardLoadoutSnapshot Before = Skills->GetSnapshot();
	FWizardLoadoutSnapshot Bad = Before;
	Bad.Slots[1] = Bad.Slots[0];
	TestEqual(TEXT("重复技能拒绝恢复"), Skills->RestoreSnapshot(Bad), EWizardSkillResult::DuplicateSkill);
	Bad.Slots[1] = TEXT("Missing");
	TestEqual(TEXT("未知技能拒绝恢复"), Skills->RestoreSnapshot(Bad), EWizardSkillResult::UnknownSkill);
	TestTrue(TEXT("失败不改任何槽"), Before.Slots == Skills->GetSnapshot().Slots);
	FWizardLoadoutSnapshot Empty;
	Empty.Slots.Init(NAME_None, 4);
	TestEqual(TEXT("允许四槽全空快照"), Skills->RestoreSnapshot(Empty), EWizardSkillResult::Success);
	Fixture.State->InitializeAvatar(Fixture.Avatar);
	TestTrue(TEXT("重新初始化不填回全空搭配"), Skills->GetSnapshot().Slots == Empty.Slots);
	const float EmptyMana = Fixture.State->GetAttributes()->GetMana();
	TestEqual(TEXT("空槽请求无效"), Skills->ActivateSlot(0), EWizardSkillResult::InvalidSlot);
	TestEqual(TEXT("空槽不扣法力"), Fixture.State->GetAttributes()->GetMana(), EmptyMana);
	Skills->RestoreSnapshot(Before);
	TestEqual(TEXT("离开Alchemy不允许改搭配"), Skills->EquipSkill(TEXT("Heal"), 0), EWizardSkillResult::NotAtAlchemy);
	TestEqual(TEXT("离开Alchemy不允许交换"), Skills->SwapSlots(0,1), EWizardSkillResult::NotAtAlchemy);
	UAbilitySystemComponent* ASC = Fixture.State->GetAbilitySystemComponent();
	const UWizardSkillDefinition* Fireball = Skills->Find(TEXT("Fireball"));
	FGameplayEffectSpecHandle Cooldown = ASC->MakeOutgoingSpec(UWizardSkillTimedEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
	Cooldown.Data->SetDuration(5.f, true);
	Cooldown.Data->DynamicGrantedTags.AddTag(Fireball->CooldownTag);
	ASC->ApplyGameplayEffectSpecToSelf(*Cooldown.Data.Get());
	FWizardLoadoutSnapshot Swapped = Before;
	Swapped.Slots.Swap(0,1);
	TestEqual(TEXT("有效快照可原子恢复"), Skills->RestoreSnapshot(Swapped), EWizardSkillResult::Success);
	TestTrue(TEXT("换槽保留技能冷却"), Skills->GetCooldownRemaining(Fireball) > 0.f);
	FVector Target;
	Fixture.State->SetAimPoint(FVector(500,0,0));
	TestEqual(TEXT("冷却拒绝"), Skills->ActivateSlot(1), EWizardSkillResult::CoolingDown);
	ASC->SetNumericAttributeBase(UWizardAttributeSet::GetManaAttribute(), 0.f);
	TestEqual(TEXT("资源不足拒绝"), Skills->ActivateSlot(0), EWizardSkillResult::NotEnoughMana);
	TestEqual(TEXT("失败不扣法力"), Fixture.State->GetAttributes()->GetMana(), 0.f);
	ASC->SetNumericAttributeBase(UWizardAttributeSet::GetManaAttribute(), 100.f);
	TestEqual(TEXT("满血治疗失败"), Skills->CheckSkill(Skills->Find(TEXT("Heal")), FVector(500,0,0), Target), EWizardSkillResult::FullHealth);
	TestEqual(TEXT("无效方向失败"), Skills->CheckSkill(Skills->Find(TEXT("IceArrow")), Fixture.Avatar->GetActorLocation(), Target), EWizardSkillResult::InvalidTarget);
	Skills->ApplyShield(60.f, 5.f);
	AWizardCharacter* Replacement = Fixture.World->SpawnActor<AWizardCharacter>();
	Fixture.State->InitializeAvatar(Replacement);
	TestEqual(TEXT("换Avatar清护盾"), Fixture.State->GetAttributes()->GetShield(), 0.f);
	TestTrue(TEXT("换Avatar保留搭配"), Skills->GetSnapshot().Slots == Swapped.Slots);
	TestTrue(TEXT("换Avatar保留技能冷却"), Skills->GetCooldownRemaining(Fireball) > 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardSkillEffectsTest, "Wizard.Skills.EffectsAndExpiry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardSkillEffectsTest::RunTest(const FString& Parameters)
{
	WizardSkillTests::FFixture Fixture;
	UAbilitySystemComponent* ASC = Fixture.State->GetAbilitySystemComponent();
	UWizardSkillLoadoutComponent* Skills = Fixture.State->GetSkills();
	UWizardAttributeSet* Attributes = Fixture.State->GetAttributes();
	auto Damage = [&](float Amount)
	{
		Fixture.Enemy->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetAttackPowerAttribute(), Amount);
		FGameplayEffectSpecHandle Spec = Fixture.Enemy->GetAbilitySystemComponent()->MakeOutgoingSpec(UWizardSkillDamageEffect::StaticClass(), 1.f, Fixture.Enemy->GetAbilitySystemComponent()->MakeEffectContext());
		Fixture.Enemy->GetAbilitySystemComponent()->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), ASC);
	};
	Skills->ApplyShield(60.f, 5.f);
	Damage(25.f);
	TestEqual(TEXT("护盾吸收"), Attributes->GetShield(), 35.f);
	TestEqual(TEXT("护盾保护生命"), Attributes->GetHealth(), 100.f);
	Damage(50.f);
	TestEqual(TEXT("护盾耗尽"), Attributes->GetShield(), 0.f);
	TestEqual(TEXT("溢出伤害只扣剩余"), Attributes->GetHealth(), 85.f);
	Skills->ApplyShield(60.f, 5.f);
	Skills->ApplyShield(30.f, .1f);
	TestEqual(TEXT("刷新不叠加"), Attributes->GetShield(), 30.f);
	Fixture.Advance(.3f);
	TestEqual(TEXT("护盾到期归零"), Attributes->GetShield(), 0.f);
	FGameplayEffectSpecHandle Heal = ASC->MakeOutgoingSpec(UWizardSkillHealEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
	Heal.Data->SetSetByCallerMagnitude(WizardSkills::ValueTag(), 40.f);
	ASC->ApplyGameplayEffectSpecToSelf(*Heal.Data.Get());
	TestEqual(TEXT("治疗上限"), Attributes->GetHealth(), 100.f);
	UAbilitySystemComponent* TargetASC = Fixture.Enemy->GetAbilitySystemComponent();
	FGameplayEffectSpecHandle Slow = ASC->MakeOutgoingSpec(UWizardSkillSlowEffect::StaticClass(), 1.f, ASC->MakeEffectContext());
	Slow.Data->SetDuration(.1f, true);
	Slow.Data->SetSetByCallerMagnitude(WizardSkills::ValueTag(), .5f);
	ASC->ApplyGameplayEffectSpecToTarget(*Slow.Data.Get(), TargetASC);
	TestEqual(TEXT("减速作用于实际移动组件"), Fixture.Enemy->GetCharacterMovement()->MaxWalkSpeed, 300.f);
	ASC->ApplyGameplayEffectSpecToTarget(*Slow.Data.Get(), TargetASC);
	TestEqual(TEXT("重复减速不叠乘"), Fixture.Enemy->GetAttributes()->GetMoveSpeed(), 300.f);
	Fixture.Advance(.3f);
	TestEqual(TEXT("减速到期恢复属性"), Fixture.Enemy->GetAttributes()->GetMoveSpeed(), 600.f);
	TestEqual(TEXT("减速到期恢复移动组件"), Fixture.Enemy->GetCharacterMovement()->MaxWalkSpeed, 600.f);
	Skills->ApplyShield(60.f, 5.f);
	ASC->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 0.f);
	TestEqual(TEXT("死亡清护盾"), Attributes->GetShield(), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardSkillTargetsTest, "Wizard.Skills.TargetsAndCampTransactions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardSkillTargetsTest::RunTest(const FString& Parameters)
{
	WizardSkillTests::FFixture Fixture(true);
	auto BoxActor = [&](FVector Location, FVector Extent)
	{
		AActor* Actor = Fixture.World->SpawnActor<AActor>();
		UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
		Actor->SetRootComponent(Box);
		Box->SetBoxExtent(Extent);
		Box->SetCollisionProfileName(TEXT("BlockAll"));
		Box->SetCollisionObjectType(ECC_WorldStatic);
		Box->RegisterComponent();
		Actor->SetActorLocation(Location);
		return Actor;
	};
	BoxActor(FVector(0,0,-25), FVector(2000,2000,25));
	UWizardSkillLoadoutComponent* Skills = Fixture.State->GetSkills();
	FVector Target;
	const UWizardSkillDefinition* Blink = Skills->Find(TEXT("Blink"));
	TestEqual(TEXT("开放地面闪现有效"), Skills->CheckSkill(Blink, FVector(500,0,100), Target), EWizardSkillResult::Success);
	TestTrue(TEXT("闪现距离受配置限制"), FVector::Dist2D(Fixture.Avatar->GetActorLocation(), Target) <= Blink->Range + .1f);
	TestEqual(TEXT("超距闪现截取到距离上限"), Skills->CheckSkill(Blink, FVector(1500,0,100), Target), EWizardSkillResult::Success);
	TestTrue(TEXT("截取后不超过距离上限"), FVector::Dist2D(Fixture.Avatar->GetActorLocation(), Target) <= Blink->Range + .1f);
	AActor* Wall = BoxActor(FVector(250,0,180), FVector(25,300,180));
	TestEqual(TEXT("闪现不能穿墙"), Skills->CheckSkill(Blink, FVector(500,0,100), Target), EWizardSkillResult::InvalidTarget);
	TestEqual(TEXT("雷击不能越过场景遮挡"), Skills->CheckSkill(Skills->Find(TEXT("Lightning")), FVector(500,0,100), Target), EWizardSkillResult::InvalidTarget);
	Wall->Destroy();
	TestEqual(TEXT("雷击超距离拒绝"), Skills->CheckSkill(Skills->Find(TEXT("Lightning")), FVector(1800,0,100), Target), EWizardSkillResult::InvalidTarget);
	TestEqual(TEXT("零方向闪现拒绝"), Skills->CheckSkill(Blink, Fixture.Avatar->GetActorLocation(), Target), EWizardSkillResult::InvalidTarget);
	Fixture.Avatar->SetActorLocation(FVector(3000,0,100));
	TestEqual(TEXT("没有地面拒绝闪现"), Skills->CheckSkill(Blink, FVector(3500,0,100), Target), EWizardSkillResult::InvalidTarget);
	Fixture.Avatar->SetActorLocation(FVector(0,0,100));
	AWizardAlchemyStation* Station = Fixture.World->SpawnActor<AWizardAlchemyStation>();
	AActor* Alchemy = BoxActor(FVector(150,0,65), FVector(30,30,65));
	Station->AlchemyActor = Alchemy;
	Station->InitializeInteraction();
	Fixture.State->GetInteraction()->RegisterTarget(Station->InteractionTarget);
	Fixture.Advance(.1f);
	FVector AlchemyCenter, AlchemyExtent;
	Alchemy->GetActorBounds(false, AlchemyCenter, AlchemyExtent);
	FHitResult SightHit;
	FCollisionQueryParams SightQuery(SCENE_QUERY_STAT(SkillCampTest), false, Fixture.Avatar);
	SightQuery.AddIgnoredActor(Station);
	Fixture.World->LineTraceSingleByChannel(SightHit, Fixture.Avatar->GetActorLocation(), AlchemyCenter, ECC_Visibility, SightQuery);
	AddInfo(FString::Printf(TEXT("Camp fixture: Avatar=%s Alchemy=%s Center=%s Station=%s HP=%.1f LOS=%s ExpectedActor=%s"),
		*Fixture.Avatar->GetActorLocation().ToString(), *Alchemy->GetActorLocation().ToString(), *AlchemyCenter.ToString(), *Station->GetActorLocation().ToString(),
		Fixture.State->GetAttributes()->GetHealth(), *GetNameSafe(SightHit.GetActor()), *GetNameSafe(Alchemy)));
	TestTrue(TEXT("营地交互有效"), Station->CanInteract(Fixture.State));
	{
		const FWizardLoadoutSnapshot Original = Skills->GetSnapshot();
		UWizardSkillMenuController* Controller = NewObject<UWizardSkillMenuController>();
		TestTrue(TEXT("MVC会话绑定模型"), Controller->Open(Skills));
		int32 Refreshes = 0;
		Controller->OnChanged.AddLambda([&Refreshes]() { ++Refreshes; });
		FWizardSkillDragRequest Stale{Original.Slots[0], 0, Controller};
		TestTrue(TEXT("MVC候选拖入替换"), Controller->HandleDrop({TEXT("Heal"), INDEX_NONE, Controller}, 0));
		TestEqual(TEXT("MVC有效变更仅刷新一次"), Refreshes, 1);
		TestFalse(TEXT("MVC已装备候选禁止拖拽"), Controller->CanDragSkill(TEXT("Heal"), INDEX_NONE));
		TestFalse(TEXT("MVC拒绝过期来源槽身份"), Controller->HandleDrop(Stale, 1));
		TestTrue(TEXT("MVC槽位交换"), Controller->HandleDrop({TEXT("Heal"), 0, Controller}, 1));
		TestTrue(TEXT("MVC拖回候选区卸下"), Controller->HandleDrop({TEXT("Heal"), 1, Controller}, INDEX_NONE));
		TArray<FWizardSkillMenuEntry> Candidates, Slots;
		Controller->GetEntries(Candidates, Slots);
		TestEqual(TEXT("MVC保留四槽"), Slots.Num(), 4);
		TestNull(TEXT("MVC空槽显示数据"), Slots[1].Definition);
		TestEqual(TEXT("MVC候选完整"), Candidates.Num(), Skills->Catalog->Skills.Num());
		FWizardSkillDragRequest OldSession{TEXT("Heal"), INDEX_NONE, Controller};
		Controller->Close();
		const int32 BeforeClose = Refreshes;
		TestFalse(TEXT("MVC关闭后拒绝拖拽"), Controller->HandleDrop(OldSession, 0));
		UWizardSkillMenuController* Next = NewObject<UWizardSkillMenuController>();
		TestTrue(TEXT("MVC重开新会话"), Next->Open(Skills));
		TestFalse(TEXT("MVC新页面拒绝旧会话拖拽"), Next->HandleDrop(OldSession, 0));
		Fixture.Avatar->SetActorLocation(FVector(800,0,100));
		EWizardSkillResult Feedback = EWizardSkillResult::Success;
		Next->OnFeedback.AddLambda([&Feedback](EWizardSkillResult Result) { Feedback = Result; });
		const FWizardLoadoutSnapshot BeforeDenied = Skills->GetSnapshot();
		TestTrue(TEXT("MVC消费业务拒绝并反馈"), Next->HandleDrop({TEXT("Heal"), INDEX_NONE, Next}, 0));
		TestEqual(TEXT("MVC提交重验营地权限"), Feedback, EWizardSkillResult::NotAtAlchemy);
		TestTrue(TEXT("MVC失败保持模型"), Skills->GetSnapshot().Slots == BeforeDenied.Slots);
		Next->Close();
		Skills->RestoreSnapshot(Original);
		TestEqual(TEXT("MVC关闭后解除模型订阅"), Refreshes, BeforeClose);
		Fixture.Avatar->SetActorLocation(FVector(0,0,100));
	}

	TestEqual(TEXT("候选替换立即成功"), Skills->EquipSkill(TEXT("Heal"), 0), EWizardSkillResult::Success);
	TestEqual(TEXT("重复装备拒绝"), Skills->EquipSkill(TEXT("Heal"), 1), EWizardSkillResult::DuplicateSkill);
	TestEqual(TEXT("有效槽位交换"), Skills->SwapSlots(0,1), EWizardSkillResult::Success);
	TestEqual(TEXT("交换后治疗位于第二槽"), Skills->GetSnapshot().Slots[1], FName(TEXT("Heal")));
	int32 Changes = 0;
	const FDelegateHandle ChangeHandle = Skills->OnLoadoutChanged.AddLambda([&Changes]() { ++Changes; });
	TestEqual(TEXT("营地允许卸下"), Skills->UnequipSkill(0), EWizardSkillResult::Success);
	TestNull(TEXT("卸下形成空槽"), Skills->GetSlot(0));
	TestEqual(TEXT("卸下事件一次"), Changes, 1);
	TestEqual(TEXT("重复卸下空槽幂等"), Skills->UnequipSkill(0), EWizardSkillResult::Success);
	TestEqual(TEXT("重复卸下不广播"), Changes, 1);
	TestEqual(TEXT("非法卸下槽拒绝"), Skills->UnequipSkill(4), EWizardSkillResult::InvalidSlot);
	TestEqual(TEXT("技能可交换至空槽"), Skills->SwapSlots(1,0), EWizardSkillResult::Success);
	TestNull(TEXT("原槽交换后为空"), Skills->GetSlot(1));
	TestEqual(TEXT("候选可装备至空槽"), Skills->EquipSkill(TEXT("Fireball"), 1), EWizardSkillResult::Success);
	TestEqual(TEXT("装备仍拒绝重复"), Skills->EquipSkill(TEXT("Fireball"), 2), EWizardSkillResult::DuplicateSkill);
	Skills->OnLoadoutChanged.Remove(ChangeHandle);
	Fixture.Avatar->SetActorLocation(FVector(800,0,100));
	const auto Before = Skills->GetSnapshot();
	TestEqual(TEXT("离开交互范围拒绝提交"), Skills->EquipSkill(TEXT("Shield"), 0), EWizardSkillResult::NotAtAlchemy);
	TestEqual(TEXT("离开交互范围拒绝卸下"), Skills->UnequipSkill(0), EWizardSkillResult::NotAtAlchemy);
	TestTrue(TEXT("无效提交不改搭配"), Skills->GetSnapshot().Slots == Before.Slots);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardInteractionEventsTest, "Wizard.Interaction.EventsAndLifetime", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardInteractionEventsTest::RunTest(const FString& Parameters)
{
	WizardSkillTests::FFixture Fixture(true);
	Fixture.Avatar->SetPlayerState(Fixture.State);
	Fixture.Avatar->SetActorLocation(FVector(1000,0,100));
	UWizardInteractionComponent* Interaction = Fixture.State->GetInteraction();
	int32 Events = 0, FirstExecutions = 0, ShopExecutions = 0;
	const FDelegateHandle PromptHandle = Interaction->OnPromptChanged.AddLambda([&Events](const FText&) { ++Events; });
	auto MakeTarget = [&](FVector Location, const TCHAR* Prompt, int32& Executions)
	{
		AActor* Actor = Fixture.World->SpawnActor<AActor>();
		UBoxComponent* Root = NewObject<UBoxComponent>(Actor);
		Actor->SetRootComponent(Root);
		Root->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Root->RegisterComponent();
		Actor->SetActorLocation(Location);
		UWizardInteractionTarget* Target = NewObject<UWizardInteractionTarget>(Actor);
		Target->SetupAttachment(Root);
		Target->RegisterComponent();
		Target->Action.BindLambda([&Executions](AWizardPlayerState*) { ++Executions; return true; });
		Target->InitializeTarget(Actor, 250.f, FText::FromString(Prompt));
		return Target;
	};
	UWizardInteractionTarget* First = MakeTarget(FVector(0,0,100), TEXT("E 技能搭配"), FirstExecutions);
	UWizardInteractionTarget* Shop = MakeTarget(FVector(400,0,100), TEXT("E 商店"), ShopExecutions);
	TestTrue(TEXT("范围外没有提示"), Interaction->GetPrompt().IsEmpty());
	Fixture.Avatar->SetActorLocation(FVector(100,0,100));
	TestTrue(TEXT("进入范围由Overlap立即选择目标"), Interaction->GetCurrentTarget() == First);
	const int32 EnterEvents = Events;
	Fixture.Avatar->SetActorLocation(FVector(120,0,100));
	TestEqual(TEXT("提示未变不重复广播"), Events, EnterEvents);
	TestTrue(TEXT("统一入口执行第一种业务"), Interaction->TryInteract());
	TestEqual(TEXT("第一业务只执行一次"), FirstExecutions, 1);
	Fixture.Avatar->SetActorLocation(FVector(230,0,100));
	TestTrue(TEXT("同时处于两范围时选择较近目标"), Interaction->GetCurrentTarget() == Shop);
	TestTrue(TEXT("统一入口执行商店适配，无技能分支"), Interaction->TryInteract());
	TestEqual(TEXT("商店适配只执行一次"), ShopExecutions, 1);
	Interaction->SetSuppressed(true);
	TestTrue(TEXT("菜单打开隐藏提示"), Interaction->GetPrompt().IsEmpty());
	TestFalse(TEXT("菜单打开禁止交互"), Interaction->TryInteract());
	Interaction->SetSuppressed(false);
	TestTrue(TEXT("菜单关闭恢复目标"), Interaction->GetCurrentTarget() == Shop);
	Fixture.State->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 0.f);
	TestTrue(TEXT("死亡事件清提示"), Interaction->GetPrompt().IsEmpty());
	Fixture.State->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 100.f);
	TestTrue(TEXT("恢复生命事件重建提示"), Interaction->GetCurrentTarget() == Shop);
	Shop->SetInteractionEnabled(false);
	TestTrue(TEXT("目标条件变化主动切回其他有效目标"), Interaction->GetCurrentTarget() == First);
	First->GetOwner()->Destroy();
	TestTrue(TEXT("目标销毁事件清除提示"), Interaction->GetPrompt().IsEmpty());
	Shop->SetInteractionEnabled(true);
	TestTrue(TEXT("目标重新启用主动刷新"), Interaction->GetCurrentTarget() == Shop);
	Fixture.Avatar->SetActorLocation(FVector(1000,0,100));
	TestTrue(TEXT("离开范围事件清除提示"), Interaction->GetPrompt().IsEmpty());
	Fixture.Avatar->SetActorLocation(FVector(400,0,100));
	TestTrue(TEXT("重新进入恢复提示"), Interaction->GetCurrentTarget() == Shop);
	Interaction->BindAvatar(nullptr);
	TestTrue(TEXT("解除Avatar清空候选"), Interaction->GetPrompt().IsEmpty());
	Interaction->BindAvatar(Fixture.Avatar);
	TestTrue(TEXT("在范围内重绑Avatar一次性初始化重叠"), Interaction->GetCurrentTarget() == Shop);
	Interaction->OnPromptChanged.Remove(PromptHandle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardSkillAreaTest, "Wizard.Skills.AreaTargetsAndOcclusion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)
bool FWizardSkillAreaTest::RunTest(const FString& Parameters)
{
	WizardSkillTests::FFixture Fixture(true);
	Fixture.Avatar->SetActorLocation(FVector(-500,0,100));
	Fixture.Enemy->SetActorLocation(FVector(80,0,88));
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AWizardEnemy* Rear = Fixture.World->SpawnActor<AWizardEnemy>(AWizardEnemy::StaticClass(), FVector(180,0,88), FRotator::ZeroRotator, Params);
	AWizardEnemy* Outside = Fixture.World->SpawnActor<AWizardEnemy>(AWizardEnemy::StaticClass(), FVector(600,0,88), FRotator::ZeroRotator, Params);
	UAbilitySystemComponent* ASC = Fixture.State->GetAbilitySystemComponent();
	const UWizardSkillDefinition* Lightning = Fixture.State->GetSkills()->Find(TEXT("Lightning"));
	auto ResetHealth = [&]()
	{
		for (AWizardEnemy* Enemy : {Fixture.Enemy, Rear, Outside})
		{
			Enemy->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetMaxHealthAttribute(), 1000.f);
			Enemy->GetAbilitySystemComponent()->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 1000.f);
		}
	};
	ResetHealth();
	Fixture.Advance(.1f);
	const float Expected = 1000.f - 25.f * Lightning->Power;
	WizardSkills::ApplyAreaDamage(ASC, Lightning, FVector::ZeroVector, ASC->MakeEffectContext());
	TestEqual(TEXT("范围前排恰好受伤一次"), Fixture.Enemy->GetAttributes()->GetHealth(), Expected);
	TestEqual(TEXT("非致死前排不遮挡后排范围伤害"), Rear->GetAttributes()->GetHealth(), Expected);
	TestEqual(TEXT("范围外不受伤"), Outside->GetAttributes()->GetHealth(), 1000.f);
	AActor* Wall = Fixture.World->SpawnActor<AActor>();
	UBoxComponent* Box = NewObject<UBoxComponent>(Wall);
	Wall->SetRootComponent(Box);
	Box->SetBoxExtent(FVector(5,60,150));
	Box->SetCollisionProfileName(TEXT("BlockAll"));
	Box->RegisterComponent();
	Wall->SetActorLocation(FVector(130,0,100));
	ResetHealth();
	Fixture.Advance(.1f);
	WizardSkills::ApplyAreaDamage(ASC, Lightning, FVector::ZeroVector, ASC->MakeEffectContext());
	TestEqual(TEXT("墙前目标正常受伤"), Fixture.Enemy->GetAttributes()->GetHealth(), Expected);
	TestEqual(TEXT("场景墙体仍阻挡后排"), Rear->GetAttributes()->GetHealth(), 1000.f);
	return true;
}
#endif
