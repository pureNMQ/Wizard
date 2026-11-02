#include "Skills/WizardSkillCue.h"
#include "Combat/WizardCombatPresentation.h"
#include "Skills/WizardSkillDefinition.h"
#include "WizardPlayerState.h"
#include "Combat/WizardCombatConfig.h"
#include "NativeGameplayTags.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "UObject/ConstructorHelpers.h"
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SkillImpactCue, "GameplayCue.Wizard.Skill.Impact");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SkillShieldCue, "GameplayCue.Wizard.Skill.Shield");
FGameplayTag WizardSkills::ImpactCueTag()
{
	return TAG_SkillImpactCue;
}
FGameplayTag WizardSkills::ShieldCueTag()
{
	return TAG_SkillShieldCue;
}
void WizardSkills::PlayCastSound(AActor* Avatar, const UWizardSkillDefinition* Definition)
{
	if (Avatar && Definition && Definition->Presentation)
	{
		FVector Origin = Avatar->GetActorLocation();
		if (const APawn* Pawn = Cast<APawn>(Avatar))
		{
			if (const auto* State = Pawn->GetPlayerState<AWizardPlayerState>())
				if (Definition->Behavior == EWizardSkillBehavior::Projectile)
					Origin += Avatar->GetActorRotation().RotateVector(State->GetCombatConfig()->MuzzleOffset);
		}
		WizardCombat::Burst(Avatar, Definition->Presentation, EWizardCombatStage::Cast, Origin);
		return;
	}
	if (Avatar && Definition && Definition->CastSound)
		UGameplayStatics::PlaySoundAtLocation(Avatar, Definition->CastSound, Avatar->GetActorLocation(), .6f);
}
UWizardSkillImpactCue::UWizardSkillImpactCue()
{
	GameplayCueTag = WizardSkills::ImpactCueTag();
}
bool UWizardSkillImpactCue::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	const UWizardSkillDefinition* Definition = Cast<UWizardSkillDefinition>(Parameters.GetSourceObject());
	if (!MyTarget || !Definition)
		return false;
	if (Definition->Presentation)
	{
		WizardCombat::Burst(MyTarget, Definition->Presentation, EWizardCombatStage::Impact, Parameters.Location, Parameters.Normal);
		return true;
	}
	UFXSystemComponent* Spawned = nullptr;
	if (Definition->ImpactFX)
	{
		Spawned = UNiagaraFunctionLibrary::SpawnSystemAtLocation(MyTarget, Definition->ImpactFX, Parameters.Location, FRotator::ZeroRotator,
		                                                         Definition->EffectScale, true, true);
	}
	else if (Definition->CascadeFX)
	{
		Spawned = UGameplayStatics::SpawnEmitterAtLocation(MyTarget, Definition->CascadeFX, Parameters.Location, FRotator::ZeroRotator,
		                                                   Definition->EffectScale, true);
	}
	if (Spawned)
	{
		// 循环候选也有硬寿命；回调仅弱持有组件，换关卡由World清理定时器。
		FTimerHandle Expiry;
		TWeakObjectPtr<UFXSystemComponent> Weak = Spawned;
		MyTarget->GetWorld()->GetTimerManager().SetTimer(
		    Expiry,
		    [Weak]()
		    {
			    if (Weak.IsValid())
				    Weak->DestroyComponent();
		    },
		    Definition->EffectLifetime, false);
	}
	if (Definition->ImpactSound)
		UGameplayStatics::PlaySoundAtLocation(MyTarget, Definition->ImpactSound, Parameters.Location, .6f);
	return true;
}
AWizardSkillShieldCue::AWizardSkillShieldCue()
{
	GameplayCueTag = WizardSkills::ShieldCueTag();
	bAutoDestroyOnRemove = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	ShieldMesh = Sphere.Object;
}
bool AWizardSkillShieldCue::OnActive_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	// GE触发的持续Cue通过EffectContext携带技能定义；使用引擎访问器兼容直接Cue参数。
	const UWizardSkillDefinition* Definition = Cast<UWizardSkillDefinition>(Parameters.GetSourceObject());
	if (!MyTarget || !Definition)
		return false;
	if (Definition->Presentation)
	{
		if (!ShieldBody && Definition->Presentation->ShieldMaterial)
		{
			ShieldBody = NewObject<UStaticMeshComponent>(this);
			ShieldBody->SetStaticMesh(ShieldMesh);
			ShieldBody->SetMaterial(0, Definition->Presentation->ShieldMaterial);
			ShieldBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			ShieldBody->SetCastShadow(false);
			ShieldBody->SetupAttachment(MyTarget->GetRootComponent());
			ShieldBody->RegisterComponent();
			const ACharacter* Character = Cast<ACharacter>(MyTarget);
			const float HalfHeight = Character ? Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 90.f;
			// 引擎球体半径50厘米；直接附着根组件，尺寸只影响表现，不扩大碰撞或承伤范围。
			ShieldBody->SetWorldScale3D(Definition->Presentation->ShieldRadiusScale * (HalfHeight / 50.f));
		}
		if (!Effect)
			Effect = Cast<UNiagaraComponent>(WizardCombat::SpawnStyle(MyTarget, Definition->Presentation, EWizardCombatStage::Status,
			                                                          MyTarget->GetActorLocation(), true));
		return true;
	}
	if (!Definition->ImpactFX)
		return false;
	if (!Effect)
	{
		Effect =
		    UNiagaraFunctionLibrary::SpawnSystemAttached(Definition->ImpactFX, MyTarget->GetRootComponent(), NAME_None, FVector(0, 0, -90),
		                                                 FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false);
		if (Effect)
			Effect->SetRelativeScale3D(Definition->EffectScale);
	}
	return true;
}
bool AWizardSkillShieldCue::OnRemove_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters)
{
	if (Effect)
	{
		Effect->DeactivateImmediate();
		Effect->DestroyComponent();
		Effect = nullptr;
	}
	if (ShieldBody)
	{
		ShieldBody->DestroyComponent();
		ShieldBody = nullptr;
	}
	return true;
}

void AWizardSkillShieldCue::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Effect)
	{
		Effect->DestroyComponent();
		Effect = nullptr;
	}
	if (ShieldBody)
	{
		ShieldBody->DestroyComponent();
		ShieldBody = nullptr;
	}
	Super::EndPlay(Reason);
}
