#include "Combat/WizardProjectile.h"
#include "Combat/WizardCombatPresentation.h"
#include "AbilitySystemComponent.h"
#include "Combat/WizardCombatConfig.h"
#include "Combat/WizardEnemy.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "Skills/WizardSkillDefinition.h"
#include "Skills/WizardSkillEffects.h"
#include "Skills/WizardSkillCue.h"

AWizardProjectile::AWizardProjectile()
{
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(12.0f);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Collision->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	Collision->OnComponentHit.AddDynamic(this, &AWizardProjectile::OnHit);
	Trail = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Trail"));
	Trail->SetupAttachment(Collision);
	Trail->SetAutoActivate(false);
	Trail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->UpdatedComponent = Collision;
	Movement->ProjectileGravityScale = 0.0f;
	Movement->bRotationFollowsVelocity = true;
	InitialLifeSpan = 2.0f;
}

void AWizardProjectile::InitializeProjectile(UAbilitySystemComponent* Source, const FGameplayEffectSpecHandle& Spec, const UWizardCombatConfig* Config)
{
	SourceASC = Source;
	DamageSpec = Spec;
	Presentation = Config->Presentation;
	if (Presentation) { Trail->SetAsset(Presentation->Projectile); Trail->SetRelativeScale3D(Presentation->ProjectileScale); }
	Movement->InitialSpeed = FMath::Max(1.0f, Config->ProjectileSpeed);
	Movement->MaxSpeed = Movement->InitialSpeed;
	InitialLifeSpan = FMath::Max(0.1f, Config->ProjectileLifetime);
}

void AWizardProjectile::BeginPlay()
{
	Super::BeginPlay();
	Collision->IgnoreActorWhenMoving(GetOwner(), true);
	Movement->Velocity = GetActorForwardVector() * Movement->InitialSpeed;
	if (Trail->GetAsset())
	{
		WizardCombat::ApplyColors(Trail, Presentation);
	Trail->Activate(true);
	}
}

void AWizardProjectile::InitializeSkillProjectile(UAbilitySystemComponent* Source, const FGameplayEffectSpecHandle& Spec, const UWizardSkillDefinition* Definition)
{
	SourceASC = Source;
	DamageSpec = Spec;
	Skill = Definition;
	Movement->InitialSpeed = Definition->ProjectileSpeed;
	Movement->MaxSpeed = Movement->InitialSpeed;
	InitialLifeSpan = Definition->Range / Definition->ProjectileSpeed;
	Presentation = Definition->Presentation;
	Trail->SetAsset(Presentation ? Presentation->Projectile : Definition->ProjectileFX);
	Trail->SetRelativeScale3D(Presentation ? Presentation->ProjectileScale : Definition->EffectScale);
}

void AWizardProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 碰撞、寿命到期和显式销毁共用清理；不分离拖尾，避免循环系统遗留在世界中。
	if (IsValid(Trail))
	{
		Trail->DeactivateImmediate();
	}
	Super::EndPlay(EndPlayReason);
}

void AWizardProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* Other, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bResolved || Other == GetOwner() || Other == this) { return; }
	bResolved = true;
	// 普攻只对敌人结算；场景碰撞仍消耗弹丸，不能穿墙重复命中。
	AWizardEnemy* Enemy = Cast<AWizardEnemy>(Other);
	if (HasAuthority() && Enemy && !Enemy->IsDead() && SourceASC.IsValid() && DamageSpec.IsValid())
	{
		DamageSpec.Data->GetContext().AddHitResult(Hit, true);
		SourceASC->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), Enemy->GetAbilitySystemComponent());
		if (Skill && Skill->SpeedMultiplier < 1.f && !Enemy->IsDead())
		{
			FGameplayEffectSpecHandle Slow = SourceASC->MakeOutgoingSpec(UWizardSkillSlowEffect::StaticClass(), 1.f, DamageSpec.Data->GetContext());
			Slow.Data->SetDuration(Skill->Duration, true);
			Slow.Data->SetSetByCallerMagnitude(WizardSkills::ValueTag(), Skill->SpeedMultiplier);
			SourceASC->ApplyGameplayEffectSpecToTarget(*Slow.Data.Get(), Enemy->GetAbilitySystemComponent());
		}
	}
	if (Presentation)
	{
		WizardCombat::Burst(this, Presentation, Enemy ? EWizardCombatStage::Impact : EWizardCombatStage::WorldImpact, Hit.ImpactPoint, Hit.ImpactNormal);
	}
	else if (Skill && SourceASC.IsValid())
	{
		FGameplayCueParameters Cue;
		Cue.SourceObject = Skill;
		Cue.Location = Hit.ImpactPoint;
		Cue.Normal = Hit.ImpactNormal;
		SourceASC->ExecuteGameplayCue(WizardSkills::ImpactCueTag(), Cue);
	}
	Destroy();
}
