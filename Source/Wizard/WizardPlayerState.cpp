#include "WizardPlayerState.h"
#include "AbilitySystemComponent.h"
#include "Combat/WizardCombatConfig.h"
#include "GAS/WizardAttributeSet.h"
#include "GAS/WizardBasicAttackAbility.h"
#include "GAS/WizardCombatTags.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Skills/WizardSkillLoadoutComponent.h"
#include "Skills/WizardSkillEffects.h"
#include "Interaction/WizardInteractionComponent.h"

AWizardPlayerState::AWizardPlayerState()
{
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	Attributes = CreateDefaultSubobject<UWizardAttributeSet>(TEXT("Attributes"));
	Skills = CreateDefaultSubobject<UWizardSkillLoadoutComponent>(TEXT("Skills"));
	Interaction = CreateDefaultSubobject<UWizardInteractionComponent>(TEXT("Interaction"));
	static ConstructorHelpers::FObjectFinder<UWizardCombatConfig> DefaultConfig(TEXT("/Game/Wizard/Data/DA_BasicCombat.DA_BasicCombat"));
	if (DefaultConfig.Succeeded())
	{
		CombatConfig = DefaultConfig.Object;
	}
}

UAbilitySystemComponent* AWizardPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

const UWizardCombatConfig* AWizardPlayerState::GetCombatConfig() const
{
	return CombatConfig ? CombatConfig.Get() : GetDefault<UWizardCombatConfig>();
}

void AWizardPlayerState::InitializeAvatar(APawn* Avatar)
{
	if (AbilitySystem->GetAvatarActor() != Avatar)
	{
		StopBasicAttack();
		AbilitySystem->CancelAllAbilities();
		Skills->ClearAvatarEffects();
	}
	AbilitySystem->InitAbilityActorInfo(this, Avatar);
	Interaction->BindAvatar(Avatar);
	Skills->InitializeAvatar();
	if (HasAuthority() && !BasicAttackHandle.IsValid() && GetCombatConfig()->IsConfigurationValid())
	{
		BasicAttackHandle = AbilitySystem->GiveAbility(FGameplayAbilitySpec(GetCombatConfig()->BasicAttackAbilityClass, 1));
	}
	if (!HealthDelegate.IsValid())
	{
		HealthDelegate = AbilitySystem->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetHealthAttribute()).AddUObject(this, &AWizardPlayerState::OnHealthChanged);
	}
	if (!CooldownDelegate.IsValid())
	{
		CooldownDelegate = AbilitySystem->RegisterGameplayTagEvent(WizardCombat::BasicAttackCooldownTag(), EGameplayTagEventType::NewOrRemoved).AddUObject(this, &AWizardPlayerState::OnCooldownChanged);
	}
	if (!AbilityEndedDelegate.IsValid())
	{
		AbilityEndedDelegate = AbilitySystem->OnAbilityEnded.AddUObject(this, &ThisClass::OnBasicAttackEnded);
	}
}

void AWizardPlayerState::StartBasicAttack()
{
	if (!HasAuthority() || bAttackHeld || Attributes->GetHealth() <= 0.0f || !IsValid(AbilitySystem->GetAvatarActor())
		|| AbilitySystem->HasMatchingGameplayTag(WizardSkills::CastingTag()))
	{
		return;
	}
	if (!GetCombatConfig()->IsConfigurationValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("普通攻击未启动：战斗配置无效，请检查 %s"), *GetNameSafe(CombatConfig));
		return;
	}
	bAttackHeld = true;
	TryBasicAttack();
}

void AWizardPlayerState::TryBasicAttack()
{
	RetryAttackTimer.Invalidate();
	if (!bAttackHeld) { return; }
	if (Attributes->GetHealth() <= 0.0f || !IsValid(AbilitySystem->GetAvatarActor()))
	{
		StopBasicAttack();
		return;
	}
	const FGameplayAbilitySpec* Spec = AbilitySystem->FindAbilitySpecFromHandle(BasicAttackHandle);
	if (Spec && !Spec->IsActive() && !AbilitySystem->HasMatchingGameplayTag(WizardCombat::BasicAttackCooldownTag()))
	{
		AbilitySystem->TryActivateAbility(BasicAttackHandle);
	}
}

void AWizardPlayerState::ReleaseBasicAttack()
{
	bAttackHeld = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RetryAttackTimer);
	}
	RetryAttackTimer.Invalidate();
}

void AWizardPlayerState::StopBasicAttack()
{
	ReleaseBasicAttack();
	if (BasicAttackHandle.IsValid())
	{
		AbilitySystem->CancelAbilityHandle(BasicAttackHandle);
	}
}

void AWizardPlayerState::OnCooldownChanged(const FGameplayTag Tag, int32 Count)
{
	if (Count == 0 && bAttackHeld)
	{
		QueueBasicAttack();
	}
}

void AWizardPlayerState::QueueBasicAttack()
{
	if (!bAttackHeld || RetryAttackTimer.IsValid()) { return; }
	if (UWorld* World = GetWorld())
	{
		// OnAbilityEnded 在 EndAbility 栈内广播；下一帧再激活，防复用实例清理时重入。
		RetryAttackTimer = World->GetTimerManager().SetTimerForNextTick(this, &ThisClass::TryBasicAttack);
	}
}

void AWizardPlayerState::OnBasicAttackEnded(const FAbilityEndedData& Data)
{
	if (Data.AbilitySpecHandle != BasicAttackHandle) { return; }
	if (Data.bWasCancelled)
	{
		ReleaseBasicAttack();
		return;
	}
	// 冷却可能先于动画结束，也可能晚于动画；两条事件都尝试，但只有都就绪才续发。
	QueueBasicAttack();
}

void AWizardPlayerState::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	Interaction->Refresh();
	if (Data.NewValue <= 0.0f)
	{
		StopBasicAttack();
		AbilitySystem->CancelAllAbilities();
		Skills->ClearAvatarEffects();
	}
}

void AWizardPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Interaction->BindAvatar(nullptr);
	StopBasicAttack();
	AbilitySystem->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetHealthAttribute()).Remove(HealthDelegate);
	AbilitySystem->RegisterGameplayTagEvent(WizardCombat::BasicAttackCooldownTag(), EGameplayTagEventType::NewOrRemoved).Remove(CooldownDelegate);
	AbilitySystem->OnAbilityEnded.Remove(AbilityEndedDelegate);
	Super::EndPlay(EndPlayReason);
}
