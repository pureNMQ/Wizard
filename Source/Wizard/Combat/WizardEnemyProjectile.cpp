#include "Combat/WizardEnemyProjectile.h"
#include "Combat/WizardCombatPresentation.h"
#include "Combat/WizardEnemy.h"
#include "NiagaraComponent.h"
#include "Challenge/WizardChallengeArena.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "AbilitySystemComponent.h"
#include "WizardCharacter.h"
#include "GAS/WizardAttributeSet.h"
#include "UObject/ConstructorHelpers.h"
AWizardEnemyProjectile::AWizardEnemyProjectile()
{
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(14);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Collision->OnComponentHit.AddDynamic(this, &ThisClass::OnHit);
	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->ProjectileGravityScale = 0;
	Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
	Visual->SetupAttachment(Collision);
	Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Visual->SetRelativeScale3D(FVector(.28f));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (Sphere.Succeeded()) { Visual->SetStaticMesh(Sphere.Object); }
	Trail = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Trail"));
	Trail->SetupAttachment(Collision); Trail->SetAutoActivate(false);
	InitialLifeSpan = 4;
}
void AWizardEnemyProjectile::Initialize(UAbilitySystemComponent* Source, const FGameplayEffectSpecHandle& Spec, float Speed)
{
	SourceASC = Source;
	const AWizardEnemy* Enemy = Source ? Cast<AWizardEnemy>(Source->GetAvatarActor()) : nullptr;
	SourceChallenge = Enemy ? Enemy->SourceChallenge : TWeakObjectPtr<AWizardChallengeArena>();
	DamageSpec = Spec;
	Presentation = Enemy ? Enemy->Presentation : nullptr;
	if (Presentation && Presentation->Projectile) { Visual->SetVisibility(false); Trail->SetAsset(Presentation->Projectile); Trail->SetRelativeScale3D(Presentation->ProjectileScale); }
	Movement->InitialSpeed = FMath::Max(1.f, Speed);
	Movement->MaxSpeed = Movement->InitialSpeed;
}
bool AWizardEnemyProjectile::BelongsToChallenge(const AWizardChallengeArena* Challenge) const
{
	return Challenge && SourceChallenge == TWeakObjectPtr<AWizardChallengeArena>(const_cast<AWizardChallengeArena*>(Challenge));
}
void AWizardEnemyProjectile::BeginPlay()
{
	Super::BeginPlay();
	Collision->IgnoreActorWhenMoving(GetOwner(), true);
	Movement->Velocity = GetActorForwardVector() * Movement->InitialSpeed;
	WizardCombat::ApplyColors(Trail, Presentation);
	if (Trail->GetAsset()) Trail->Activate(true);
}
void AWizardEnemyProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* Other, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bResolved || Other == GetOwner() || Other == this) { return; }
	bResolved = true;
	AWizardCharacter* Player = Cast<AWizardCharacter>(Other);
	UAbilitySystemComponent* TargetASC = Player ? Player->GetAbilitySystemComponent() : nullptr;
	if (HasAuthority() && TargetASC && SourceASC.IsValid() && DamageSpec.IsValid()
		&& TargetASC->GetNumericAttribute(UWizardAttributeSet::GetHealthAttribute()) > 0)
	{
		DamageSpec.Data->GetContext().AddHitResult(Hit, true);
		SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), TargetASC);
	}
	WizardCombat::Burst(this, Presentation, Player ? EWizardCombatStage::Impact : EWizardCombatStage::WorldImpact, Hit.ImpactPoint, Hit.ImpactNormal);
	Destroy();
}
