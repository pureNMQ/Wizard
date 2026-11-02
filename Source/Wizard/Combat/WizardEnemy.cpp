#include "Combat/WizardEnemy.h"
#include "Combat/WizardCombatPresentation.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "GAS/WizardAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Combat/WizardEnemyAI.h"
#include "Combat/WizardEnemyAttackAbility.h"
#include "Components/SkeletalMeshComponent.h"
#include "WizardCharacter.h"
#include "BrainComponent.h"
#include "Animation/AnimSequence.h"
#include "Components/WidgetComponent.h"
#include "UI/WizardEnemyHealthBar.h"
#include "Loot/WizardDropConfig.h"

AWizardEnemy::AWizardEnemy()
{
	PrimaryActorTick.bCanEverTick = false;
	AIControllerClass = AWizardEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	Attributes = CreateDefaultSubobject<UWizardAttributeSet>(TEXT("Attributes"));
	EnemyHealthBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("EnemyHealthBar"));
	EnemyHealthBar->SetupAttachment(GetCapsuleComponent());
	EnemyHealthBar->SetWidgetSpace(EWidgetSpace::Screen);
	EnemyHealthBar->SetDrawSize(FVector2D(88.f, 10.f));
	EnemyHealthBar->SetRelativeLocation(FVector(0.f, 0.f, 130.f));
	EnemyHealthBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 88.0f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

UAbilitySystemComponent* AWizardEnemy::GetAbilitySystemComponent() const
{
	return AbilitySystem;
}

bool AWizardEnemy::IsDead() const
{
	return Attributes->GetHealth() <= 0.0f;
}

void AWizardEnemy::BeginPlay()
{
	Super::BeginPlay();
	AbilitySystem->InitAbilityActorInfo(this, this);
	AbilitySystem->SetNumericAttributeBase(UWizardAttributeSet::GetMaxHealthAttribute(), FMath::Max(1.f, InitialHealth));
	AbilitySystem->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), FMath::Max(1.f, InitialHealth));
	AbilitySystem->SetNumericAttributeBase(UWizardAttributeSet::GetAttackPowerAttribute(), FMath::Max(0.f, InitialAttackPower));
	AbilitySystem->SetNumericAttributeBase(UWizardAttributeSet::GetDefenseAttribute(), FMath::Max(0.f, InitialDefense));
	AbilitySystem->SetNumericAttributeBase(UWizardAttributeSet::GetMoveSpeedAttribute(), FMath::Max(1.f, InitialMoveSpeed));
	if (IsConfigurationValid())
	{
		AttackHandle = AbilitySystem->GiveAbility(FGameplayAbilitySpec(UWizardEnemyAttackAbility::StaticClass(), 1, INDEX_NONE, this));
	}
	else if (BehaviorTree)
	{
		UE_LOG(LogTemp, Error, TEXT("敌人战斗配置无效，禁止启动攻击：%s"), *GetPathName());
	}
	GetCharacterMovement()->MaxWalkSpeed = Attributes->GetMoveSpeed();
	HealthDelegate = AbilitySystem->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetHealthAttribute()).AddUObject(this, &AWizardEnemy::OnHealthChanged);
	WizardCombat::Burst(this, Presentation, EWizardCombatStage::Spawn, GetActorLocation());
	EnemyHealthBar->InitWidget();
	if (UWizardEnemyHealthBar* Widget = Cast<UWizardEnemyHealthBar>(EnemyHealthBar->GetUserWidgetObject())) { Widget->BindEnemy(this); }
	if (AWizardEnemyAIController* AI = Cast<AWizardEnemyAIController>(GetController())) { AI->StartEnemyBehavior(); }
}

void AWizardEnemy::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (!bDeathHandled && Data.OldValue > 0.0f && Data.NewValue <= 0.0f)
	{
		bDeathHandled = true;
		if (DropConfig) DropConfig->SpawnDrops(GetWorld(), GetActorLocation(), SourceChallenge.Get());
		AbilitySystem->CancelAllAbilities();
		AbilitySystem->RemoveGameplayCue(WizardCombat::SlowTag());
		if (AAIController* AI = Cast<AAIController>(GetController()))
		{
			AI->StopMovement();
			if (AI->GetBrainComponent()) { AI->GetBrainComponent()->StopLogic(TEXT("Enemy defeated")); }
		}
		GetCharacterMovement()->DisableMovement();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		OnDeath();
		if (DeathAnimation) { GetMesh()->PlayAnimation(DeathAnimation, false); }
		OnEnemyDefeated.Broadcast(this);
		// 保留短暂死亡反馈，HUD 先接收到零生命事件，再销毁目标。
		SetLifeSpan(1.5f);
	}
}

void AWizardEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AbilitySystem->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetHealthAttribute()).Remove(HealthDelegate);
	AbilitySystem->CancelAllAbilities();
		AbilitySystem->RemoveGameplayCue(WizardCombat::SlowTag());
	if (UWizardEnemyHealthBar* Widget = Cast<UWizardEnemyHealthBar>(EnemyHealthBar->GetUserWidgetObject())) { Widget->BindEnemy(nullptr); }
	Super::EndPlay(EndPlayReason);
}

bool AWizardEnemy::IsConfigurationValid() const
{
	return BehaviorTree
		&& FMath::IsFinite(InitialHealth) && InitialHealth >= 1.f
		&& FMath::IsFinite(InitialAttackPower) && InitialAttackPower >= 0.f
		&& FMath::IsFinite(InitialDefense) && InitialDefense >= 0.f
		&& FMath::IsFinite(InitialMoveSpeed) && InitialMoveSpeed >= 1.f
		&& FMath::IsFinite(AttackRange) && AttackRange >= 80.f
		&& FMath::IsFinite(AttackInterval) && AttackInterval >= .1f
		&& FMath::IsFinite(AttackWindup) && AttackWindup >= .01f
		&& (!bRanged || (FMath::IsFinite(ProjectileSpeed) && ProjectileSpeed >= 1.f));
}

bool AWizardEnemy::CanAttackTarget(const AActor* Target) const
{
	const AWizardCharacter* Player = Cast<AWizardCharacter>(Target);
	const UAbilitySystemComponent* TargetASC = Player ? Player->GetAbilitySystemComponent() : nullptr;
	if (!IsConfigurationValid() || IsDead() || !IsValid(Player) || !TargetASC
		|| TargetASC->GetNumericAttribute(UWizardAttributeSet::GetHealthAttribute()) <= 0) { return false; }
	if (FVector::Dist2D(GetActorLocation(), Target->GetActorLocation()) > AttackRange) { return false; }
	const AAIController* AI = Cast<AAIController>(GetController());
	return AI && AI->LineOfSightTo(Target);
}

bool AWizardEnemy::RequestAttack(AActor* Target)
{
	if (!CanAttackTarget(Target)) { return false; }
	AttackTarget = Target;
	return AbilitySystem->TryActivateAbility(AttackHandle);
}
