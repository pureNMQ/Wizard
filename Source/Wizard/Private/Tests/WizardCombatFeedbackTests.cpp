// Wizard 项目：保护实际结算数值与反馈生命周期，避免假伤害和永久慢动作。
#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "WizardPlayerController.h"
#include "WizardCharacter.h"
#include "WizardPlayerState.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "GameplayCueSet.h"
#include "Skills/WizardSkillCue.h"
#include "Skills/WizardSkillDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/Material.h"
#include "Combat/WizardEnemy.h"
#include "Combat/WizardCombatFeedbackComponent.h"
#include "GAS/WizardAttributeSet.h"
#include "Skills/WizardSkillEffects.h"
namespace WizardFeedbackTests
{
	struct FWorld
	{
		UWorld* World;
		FWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
			                                                .AllowAudioPlayback(false)
			                                                .CreatePhysicsScene(false)
			                                                .CreateNavigation(false)
			                                                .CreateAISystem(false)
			                                                .ShouldSimulatePhysics(false);
			World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
			GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		}
		~FWorld()
		{
			World->DestroyWorld(false);
			GEngine->DestroyWorldContext(World);
		}
	};
} // namespace WizardFeedbackTests
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardFeedbackCueDiscovery, "Wizard.Combat.Feedback.CueDiscovery",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWizardFeedbackCueDiscovery::RunTest(const FString& Parameters)
{
	// 检查引擎实际用于分发的注册表；仅加载类，不启动世界或播放音画。
	auto* Manager = UAbilitySystemGlobals::Get().GetGameplayCueManager();
	Manager->FinishLoadingGameplayCueNotifies();
	const auto* CueSet = Manager->GetRuntimeCueSet();
	if (!TestNotNull(TEXT("运行时Cue注册表"), CueSet))
		return false;
	auto CheckCue = [&](FGameplayTag Tag, UClass* ExpectedBase)
	{
		const auto* Entry = CueSet->GameplayCueData.FindByPredicate(
		    [Tag](const FGameplayCueNotifyData& Data) { return Data.GameplayCueTag == Tag; });
		if (!TestNotNull(FString::Printf(TEXT("实际分发注册：%s"), *Tag.ToString()), Entry))
			return;
		const auto* CueClass = Cast<UClass>(Entry->GameplayCueNotifyObj.TryLoad());
		TestTrue(FString::Printf(TEXT("注册的Cue处理类可加载：%s"), *Tag.ToString()),
		         CueClass && CueClass->IsChildOf(ExpectedBase));
	};
	CheckCue(WizardCombat::BurstTag(), UWizardCombatBurstCue::StaticClass());
	CheckCue(WizardCombat::WindupTag(), AWizardCombatStatusCue::StaticClass());
	CheckCue(WizardCombat::SlowTag(), AWizardCombatStatusCue::StaticClass());
	CheckCue(WizardSkills::ImpactCueTag(), UWizardSkillImpactCue::StaticClass());
	CheckCue(WizardSkills::ShieldCueTag(), AWizardSkillShieldCue::StaticClass());
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardFeedbackSettlement, "Wizard.Combat.Feedback.Settlement",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWizardFeedbackSettlement::RunTest(const FString& Parameters)
{
	WizardFeedbackTests::FWorld F;
	AWizardEnemy* Target = F.World->SpawnActor<AWizardEnemy>();
	auto* ASC = Target->GetAbilitySystemComponent();
	ASC->InitAbilityActorInfo(Target, Target);
	ASC->AddAttributeSetSubobject(Target->GetAttributes());
	ASC->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 20);
	ASC->SetNumericAttributeBase(UWizardAttributeSet::GetShieldAttribute(), 15);
	FWizardCombatResult Observed;
	int32 Count = 0;
	Target->GetAttributes()->OnCombatResolved.AddLambda(
	    [&](const FWizardCombatResult& R)
	    {
		    Observed = R;
		    ++Count;
	    });
	auto Apply = [&](float Damage)
	{
		UGameplayEffect* Effect = NewObject<UGameplayEffect>();
		FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = UWizardAttributeSet::GetIncomingDamageAttribute();
		Modifier.ModifierOp = EGameplayModOp::Additive;
		Modifier.ModifierMagnitude = FScalableFloat(Damage);
		ASC->ApplyGameplayEffectToSelf(Effect, 1, ASC->MakeEffectContext());
	};
	Apply(10);
	TestEqual(TEXT("全吸收不掉血"), Observed.HealthLost, 0.f);
	TestEqual(TEXT("全吸收数字"), Observed.Absorbed, 10.f);
	TestFalse(TEXT("剩余护盾不是破盾"), Observed.bShieldBroken);
	Apply(100);
	TestEqual(TEXT("过量伤害只显示实际掉血"), Observed.HealthLost, 20.f);
	TestEqual(TEXT("剩余护盾实际吸收"), Observed.Absorbed, 5.f);
	TestTrue(TEXT("击杀"), Observed.bKilled);
	TestTrue(TEXT("破盾"), Observed.bShieldBroken);
	Apply(100);
	TestEqual(TEXT("尸体不产生第二次伤害反馈"), Count, 2);
	ASC->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 80);
	TestEqual(TEXT("恢复基础值不产生战斗反馈"), Count, 2);
	auto Context = ASC->MakeEffectContext();
	Context.AddSourceObject(NewObject<UWizardCombatPresentation>());
	auto Heal = ASC->MakeOutgoingSpec(UWizardSkillHealEffect::StaticClass(), 1, Context);
	Heal.Data->SetSetByCallerMagnitude(WizardSkills::ValueTag(), 40);
	ASC->ApplyGameplayEffectSpecToSelf(*Heal.Data);
	TestEqual(TEXT("溢出治疗只显示有效治疗"), Observed.Healed, 20.f);
	Target->GetAttributes()->OnCombatResolved.Clear();
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardFeedbackCueEffectContext, "Wizard.Combat.Feedback.CueEffectContext",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWizardFeedbackCueEffectContext::RunTest(const FString& Parameters)
{
	WizardFeedbackTests::FWorld F;
	auto* Target = F.World->SpawnActor<AWizardEnemy>();
	auto* Cue = F.World->SpawnActor<AWizardSkillShieldCue>();
	auto* Definition = NewObject<UWizardSkillDefinition>();
	auto* Profile = NewObject<UWizardCombatPresentation>();
	Profile->ShieldMaterial = NewObject<UMaterial>();
	Definition->Presentation = Profile;
	// 按真实GE构造Cue参数：SourceObject在EffectContext内，不在参数的直接字段内。
	auto* ASC = Target->GetAbilitySystemComponent();
	ASC->InitAbilityActorInfo(Target, Target);
	auto Context = ASC->MakeEffectContext();
	Context.AddSourceObject(Definition);
	auto Spec = ASC->MakeOutgoingSpec(UWizardSkillShieldDurationEffect::StaticClass(), 1.f, Context);
	FGameplayCueParameters CueParameters;
	UAbilitySystemGlobals::Get().InitGameplayCueParameters_GESpec(CueParameters, *Spec.Data);
	TestEqual(TEXT("GE保留技能来源"), CueParameters.GetSourceObject(), static_cast<const UObject*>(Definition));
	// 未进入BeginPlay的测试世界不调度BlueprintCosmetic事件，直接检查原生处理边界。
	TestTrue(TEXT("GE上下文能创建护盾持续表现"), Cue->OnActive_Implementation(Target, CueParameters));
	TArray<UStaticMeshComponent*> Meshes;
	Cue->GetComponents(Meshes);
	TestEqual(TEXT("护盾创建一个持续盾体"), Meshes.Num(), 1);
	if (Meshes.Num() == 1)
	{
		TestEqual(TEXT("盾体跟随目标根组件"), Meshes[0]->GetAttachParent(), Target->GetRootComponent());
		TestEqual(TEXT("使用配置的盾体材质"), Meshes[0]->GetMaterial(0), Profile->ShieldMaterial.Get());
		Target->SetActorLocationAndRotation(FVector(300, -150, 80), FRotator(0, 90, 0));
		TestTrue(TEXT("移动转向后盾体中心同步跟随角色"), Meshes[0]->GetComponentLocation().Equals(Target->GetActorLocation()));
		TestEqual(TEXT("视觉盾体不改变碰撞"), Meshes[0]->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	}
	Cue->OnActive_Implementation(Target, CueParameters);
	Cue->GetComponents(Meshes);
	TestEqual(TEXT("重复激活不叠加盾体"), Meshes.Num(), 1);
	Cue->OnRemove_Implementation(Target, CueParameters);
	Cue->GetComponents(Meshes);
	TestEqual(TEXT("移除护盾清除盾体"), Meshes.Num(), 0);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardFeedbackNoBusinessSideEffects, "Wizard.Combat.Feedback.NoBusinessSideEffects",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWizardFeedbackNoBusinessSideEffects::RunTest(const FString& Parameters)
{
	WizardFeedbackTests::FWorld F;
	auto* Actor = F.World->SpawnActor<AActor>();
	auto* Component = NewObject<UWizardCombatFeedbackComponent>(Actor);
	Component->RegisterComponent();
	F.World->GetWorldSettings()->SetTimeDilation(.75f);
	Component->RequestImpact(.055f, 3);
	Component->ResetFeedback();
	TestEqual(TEXT("取消尚未执行的反馈保留外部时间倍率"), F.World->GetWorldSettings()->TimeDilation, .75f);
	FWizardCombatResult R;
	R.Target = Actor;
	R.HealthLost = 10;
	WizardCombat::Report(R);
	TestEqual(TEXT("缺少表现配置不改变世界时间"), F.World->GetWorldSettings()->TimeDilation, .75f);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardFeedbackHitStop, "Wizard.Combat.Feedback.HitStopLifecycle",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWizardFeedbackHitStop::RunTest(const FString& Parameters)
{
	WizardFeedbackTests::FWorld F;
	auto* PC = F.World->SpawnActor<AWizardPlayerController>();
	auto* Pawn = F.World->SpawnActor<APawn>();
	PC->Possess(Pawn);
	auto* Feedback = PC->CombatFeedback.Get();
	Feedback->bShakeEnabled = false;
	F.World->GetWorldSettings()->SetTimeDilation(.75f);
	Feedback->RequestImpact(.035f, 2);
	Feedback->RequestImpact(.055f, 3);
	Feedback->TickComponent(.01f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("同帧请求统一应用倍率"), F.World->GetWorldSettings()->TimeDilation, .075f);
	for (int32 I = 0; I < 10; ++I)
		Feedback->RequestImpact(.08f, 3);
	++GFrameCounter;
	F.World->Tick(LEVELTICK_All, .06f);
	Feedback->TickComponent(.001f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("按真实时间结束且命中不延长"), F.World->GetWorldSettings()->TimeDilation, .75f);
	Feedback->RequestImpact(.055f, 3);
	Feedback->TickComponent(.01f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("恢复后触发间隔"), F.World->GetWorldSettings()->TimeDilation, .75f);
	++GFrameCounter;
	F.World->Tick(LEVELTICK_All, .2f);
	Feedback->RequestImpact(.055f, 3);
	Feedback->TickComponent(.01f, LEVELTICK_All, nullptr);
	TestEqual(TEXT("间隔后可再次顿帧"), F.World->GetWorldSettings()->TimeDilation, .075f);
	PC->UnPossess();
	TestEqual(TEXT("角色解除占有恢复原倍率"), F.World->GetWorldSettings()->TimeDilation, .75f);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardPlayerDeathPresentation, "Wizard.Combat.Feedback.PlayerDeathRecovery",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWizardPlayerDeathPresentation::RunTest(const FString& Parameters)
{
	WizardFeedbackTests::FWorld F;
	UClass* PlayerClass = LoadClass<AWizardCharacter>(nullptr, TEXT("/Game/Wizard/Blueprints/BP_WizardCharacter.BP_WizardCharacter_C"));
	if (!TestNotNull(TEXT("玩家蓝图"), PlayerClass)) return false;
	auto* Avatar = F.World->SpawnActor<AWizardCharacter>(PlayerClass);
	auto* Player = F.World->SpawnActor<AWizardPlayerState>();
	Player->InitializeAvatar(Avatar);
	auto* ASC = Player->GetAbilitySystemComponent();
	ASC->AddAttributeSetSubobject(Player->GetAttributes());
	Avatar->GetMesh()->InitAnim(true);
	auto* Anim = Avatar->GetMesh()->GetAnimInstance();
	if (!TestNotNull(TEXT("玩家动画实例"), Anim) || !TestNotNull(TEXT("死亡动画配置"), Avatar->DeathMontage.Get())) return false;
	ASC->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 100.f);
	for (int32 Round = 0; Round < 2; ++Round)
	{
		ASC->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), 0.f);
		TestTrue(TEXT("真实生命归零启动死亡Montage"), Anim->Montage_IsActive(Avatar->DeathMontage));
		TestFalse(TEXT("死亡动画结束不自动回站姿"), Avatar->DeathMontage->bEnableAutoBlendOut);
		Anim->Montage_SetPosition(Avatar->DeathMontage, .4f);
		Avatar->SetDeathPresentation(true);
		TestEqual(TEXT("重复死亡通知不从头播放"), Anim->Montage_GetPosition(Avatar->DeathMontage), .4f);
		Player->RestoreAtCamp();
		TestFalse(TEXT("回营解除死亡动画"), Anim->Montage_IsActive(Avatar->DeathMontage));
		TestTrue(TEXT("回营生命恢复"), Player->GetAttributes()->GetHealth() > 0.f);
	}
	return true;
}
#endif
