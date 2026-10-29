#include "Combat/WizardEnemy.h"
#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GAS/WizardAttributeSet.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AWizardEnemy::AWizardEnemy()
{
	PrimaryActorTick.bCanEverTick = false;
	AbilitySystem = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystem"));
	Attributes = CreateDefaultSubobject<UWizardAttributeSet>(TEXT("Attributes"));
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
	GetCharacterMovement()->MaxWalkSpeed = Attributes->GetMoveSpeed();
	HealthDelegate = AbilitySystem->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetHealthAttribute()).AddUObject(this, &AWizardEnemy::OnHealthChanged);
}

void AWizardEnemy::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (Data.OldValue > 0.0f && Data.NewValue <= 0.0f)
	{
		AbilitySystem->CancelAllAbilities();
		GetCharacterMovement()->DisableMovement();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		OnDeath();
		// 保留短暂死亡反馈，HUD 先接收到零生命事件，再销毁目标。
		SetLifeSpan(1.5f);
	}
}

void AWizardEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AbilitySystem->GetGameplayAttributeValueChangeDelegate(UWizardAttributeSet::GetHealthAttribute()).Remove(HealthDelegate);
	Super::EndPlay(EndPlayReason);
}
