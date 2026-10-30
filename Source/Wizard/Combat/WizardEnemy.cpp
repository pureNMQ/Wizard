#include "Combat/WizardEnemy.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GAS/WizardAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Combat/WizardEnemyConfig.h"
#include "Combat/WizardEnemyAI.h"
#include "Combat/WizardEnemyAttackAbility.h"
#include "Components/SkeletalMeshComponent.h"
#include "WizardCharacter.h"
#include "BrainComponent.h"
#include "Animation/AnimSequence.h"
#include "Components/WidgetComponent.h"
#include "UI/WizardEnemyHealthBar.h"

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
	UStaticMeshComponent* Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TargetVisual"));
	Visual->SetupAttachment(GetCapsuleComponent());
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.5f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) { Visual->SetStaticMesh(Cube.Object); }
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
	if (EnemyConfig)
	{
		AbilitySystem->SetNumericAttributeBase(UWizardAttributeSet::GetMaxHealthAttribute(), FMath::Max(1.f, EnemyConfig->Health));
		AbilitySystem->SetNumericAttributeBase(UWizardAttributeSet::GetHealthAttribute(), FMath::Max(1.f, EnemyConfig->Health));
		AbilitySystem->SetNumericAttributeBase(UWizardAttributeSet::GetAttackPowerAttribute(), FMath::Max(0.f, EnemyConfig->AttackPower));
		AbilitySystem->SetNumericAttributeBase(UWizardAttributeSet::GetDefenseAttribute(), FMath::Max(0.f, EnemyConfig->Defense));
		AbilitySystem->SetNumericAttributeBase(UWizardAttributeSet::GetMoveSpeedAttribute(), FMath::Max(1.f, EnemyConfig->MoveSpeed));
		if (EnemyConfig->SkeletalMesh)
		{
			GetMesh()->SetSkeletalMesh(EnemyConfig->SkeletalMesh);
			GetMesh()->SetRelativeLocation(EnemyConfig->MeshOffset);
			GetMesh()->SetRelativeRotation(EnemyConfig->MeshRotation);
			GetMesh()->SetRelativeScale3D(EnemyConfig->MeshScale);
			GetMesh()->SetAnimInstanceClass(EnemyConfig->AnimClass);
			if (UStaticMeshComponent* Visual = FindComponentByClass<UStaticMeshComponent>()) { Visual->SetVisibility(false); }
		}
		AttackHandle = AbilitySystem->GiveAbility(FGameplayAbilitySpec(UWizardEnemyAttackAbility::StaticClass(), 1, INDEX_NONE, this));
	}
	GetCharacterMovement()->MaxWalkSpeed = Attributes->GetMoveSpeed();
	HealthDelegate = AbilitySystem->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetHealthAttribute()).AddUObject(this, &AWizardEnemy::OnHealthChanged);
	EnemyHealthBar->InitWidget();
	if (UWizardEnemyHealthBar* Widget = Cast<UWizardEnemyHealthBar>(EnemyHealthBar->GetUserWidgetObject())) { Widget->BindEnemy(this); }
	if (AWizardEnemyAIController* AI = Cast<AWizardEnemyAIController>(GetController())) { AI->StartEnemyBehavior(); }
}

void AWizardEnemy::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (!bDeathHandled && Data.OldValue > 0.0f && Data.NewValue <= 0.0f)
	{
		bDeathHandled = true;
		AbilitySystem->CancelAllAbilities();
		if (AAIController* AI = Cast<AAIController>(GetController()))
		{
			AI->StopMovement();
			if (AI->GetBrainComponent()) { AI->GetBrainComponent()->StopLogic(TEXT("Enemy defeated")); }
		}
		GetCharacterMovement()->DisableMovement();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		OnDeath();
		if (EnemyConfig && EnemyConfig->DeathAnimation) { GetMesh()->PlayAnimation(EnemyConfig->DeathAnimation, false); }
		OnEnemyDefeated.Broadcast(this);
		// 保留短暂死亡反馈，HUD 先接收到零生命事件，再销毁目标。
		SetLifeSpan(1.5f);
	}
}

void AWizardEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AbilitySystem->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetHealthAttribute()).Remove(HealthDelegate);
	AbilitySystem->CancelAllAbilities();
	if (UWizardEnemyHealthBar* Widget = Cast<UWizardEnemyHealthBar>(EnemyHealthBar->GetUserWidgetObject())) { Widget->BindEnemy(nullptr); }
	Super::EndPlay(EndPlayReason);
}

bool AWizardEnemy::CanAttackTarget(const AActor* Target) const
{
	const AWizardCharacter* Player = Cast<AWizardCharacter>(Target);
	const UAbilitySystemComponent* TargetASC = Player ? Player->GetAbilitySystemComponent() : nullptr;
	if (!EnemyConfig || IsDead() || !IsValid(Player) || !TargetASC
		|| TargetASC->GetNumericAttribute(UWizardAttributeSet::GetHealthAttribute()) <= 0) { return false; }
	if (FVector::Dist2D(GetActorLocation(), Target->GetActorLocation()) > EnemyConfig->AttackRange) { return false; }
	const AAIController* AI = Cast<AAIController>(GetController());
	return AI && AI->LineOfSightTo(Target);
}

bool AWizardEnemy::RequestAttack(AActor* Target)
{
	if (!CanAttackTarget(Target)) { return false; }
	AttackTarget = Target;
	return AbilitySystem->TryActivateAbility(AttackHandle);
}
