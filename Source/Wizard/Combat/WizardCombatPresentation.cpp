#include "Combat/WizardCombatPresentation.h"
#include "Combat/WizardCombatFeedbackComponent.h"
#include "Combat/WizardCombatConfig.h"
#include "Combat/WizardEnemy.h"
#include "Skills/WizardSkillDefinition.h"
#include "WizardCharacter.h"
#include "WizardPlayerController.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"
#include "AbilitySystemComponent.h"
#include "NativeGameplayTags.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_CombatBurst, "GameplayCue.Wizard.Combat.Burst");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_CombatWindup, "GameplayCue.Wizard.Combat.Windup");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_CombatSlow, "GameplayCue.Wizard.Combat.Slow");
FGameplayTag WizardCombat::BurstTag()
{
	return TAG_CombatBurst;
}
FGameplayTag WizardCombat::WindupTag()
{
	return TAG_CombatWindup;
}
FGameplayTag WizardCombat::SlowTag()
{
	return TAG_CombatSlow;
}
const UWizardCombatPresentation* WizardCombat::ResolvePresentation(const UObject* Source)
{
	if (const auto* Profile = Cast<UWizardCombatPresentation>(Source))
		return Profile;
	if (const auto* Skill = Cast<UWizardSkillDefinition>(Source))
		return Skill->Presentation;
	if (const auto* Config = Cast<UWizardCombatConfig>(Source))
		return Config->Presentation;
	if (const auto* Enemy = Cast<AWizardEnemy>(Source))
		return Enemy->Presentation;
	if (const auto* Player = Cast<AWizardCharacter>(Source))
		return Player->Presentation;
	return nullptr;
}
void WizardCombat::ApplyColors(UNiagaraComponent* Component, const UWizardCombatPresentation* Profile)
{
	if (Component && Profile)
		for (const auto& Pair : Profile->ColorOverrides)
			Component->SetVariableLinearColor(Pair.Key, Pair.Value);
}
void WizardCombat::Burst(AActor* Target, const UWizardCombatPresentation* Profile, EWizardCombatStage Stage, FVector Location,
                         FVector Normal)
{
	if (!IsValid(Target) || !Profile)
		return;
	FGameplayCueParameters P;
	P.SourceObject = Profile;
	P.Location = Location;
	P.Normal = Normal;
	P.RawMagnitude = static_cast<float>(Stage);
	// 使用相同的Cue管理器，挑战对象没有ASC时仍能分发表现。
	UAbilitySystemGlobals::Get().GetGameplayCueManager()->HandleGameplayCue(Target, BurstTag(), EGameplayCueEvent::Executed, P);
}
UFXSystemComponent* WizardCombat::SpawnStyle(AActor* Target, const UWizardCombatPresentation* Profile, EWizardCombatStage Stage,
                                             FVector Location, bool bAttached, FVector Normal)
{
	if (!IsValid(Target) || !Profile)
		return nullptr;
	const auto* Style = Profile->Stages.Find(Stage);
	if (!Style)
		return nullptr;
	UFXSystemComponent* FX = nullptr;
	const FRotator Rotation = FRotationMatrix::MakeFromZ(Normal.GetSafeNormal()).Rotator();
	if (Style->Niagara)
	{
		FX = bAttached
		         ? UNiagaraFunctionLibrary::SpawnSystemAttached(Style->Niagara, Target->GetRootComponent(), NAME_None, Style->Offset,
		                                                        FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, false)
		         : UNiagaraFunctionLibrary::SpawnSystemAtLocation(Target, Style->Niagara, Location + Style->Offset, Rotation, Style->Scale);
	}
	else if (Style->Cascade)
	{
		FX = bAttached ? UGameplayStatics::SpawnEmitterAttached(Style->Cascade, Target->GetRootComponent(), NAME_None, Style->Offset)
		               : UGameplayStatics::SpawnEmitterAtLocation(Target, Style->Cascade, Location + Style->Offset, Rotation, Style->Scale);
	}
	if (FX)
	{
		FX->SetRelativeScale3D(Style->Scale);
		ApplyColors(Cast<UNiagaraComponent>(FX), Profile);
		if (!bAttached)
		{
			FTimerHandle Timer;
			TWeakObjectPtr<UFXSystemComponent> Weak = FX;
			Target->GetWorldTimerManager().SetTimer(
			    Timer,
			    [Weak]
			    {
				    if (Weak.IsValid())
					    Weak->DestroyComponent();
			    },
			    FMath::Max(.01f, Style->Lifetime), false);
		}
	}
	if (Style->Sound)
		UGameplayStatics::PlaySoundAtLocation(Target, Style->Sound, Location, Style->Volume);
	return FX;
}
void WizardCombat::Report(const FWizardCombatResult& R)
{
	if (!IsValid(R.Target))
		return;
	const auto* TargetProfile = ResolvePresentation(R.Target);
	if (R.bKilled)
		Burst(R.Target, TargetProfile, EWizardCombatStage::Death, R.Location);
	else if (R.HealthLost > 0)
		Burst(R.Target, TargetProfile, EWizardCombatStage::Hurt, R.Location);
	if (R.bShieldBroken)
		Burst(R.Target, TargetProfile, EWizardCombatStage::ShieldBreak, R.Location);
	else if (R.Absorbed > 0)
		Burst(R.Target, TargetProfile, EWizardCombatStage::ShieldHit, R.Location);
	if (auto* PC = Cast<AWizardPlayerController>(UGameplayStatics::GetPlayerController(R.Target, 0)))
	{
		if (PC->CombatFeedback)
			PC->CombatFeedback->Consume(R);
	}
}
UWizardCombatBurstCue::UWizardCombatBurstCue()
{
	GameplayCueTag = WizardCombat::BurstTag();
}
bool UWizardCombatBurstCue::OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& P) const
{
	const auto* Profile = Cast<UWizardCombatPresentation>(P.GetSourceObject());
	const auto Stage = static_cast<EWizardCombatStage>(FMath::RoundToInt(P.RawMagnitude));
	WizardCombat::SpawnStyle(Target, Profile, Stage, P.Location, false, P.Normal);
	if (Target && Profile && Profile->FlashMaterial && (Stage == EWizardCombatStage::Hurt || Stage == EWizardCombatStage::Death))
	{
		if (auto* Character = Cast<ACharacter>(Target))
		{
			auto* Mesh = Character->GetMesh();
			Mesh->SetOverlayMaterial(Profile->FlashMaterial);
			FTimerHandle Timer;
			TWeakObjectPtr<USkeletalMeshComponent> Weak = Mesh;
			Target->GetWorldTimerManager().SetTimer(
			    Timer,
			    [Weak]
			    {
				    if (Weak.IsValid())
					    Weak->SetOverlayMaterial(nullptr);
			    },
			    .075f, false);
		}
	}
	return Profile != nullptr;
}
AWizardCombatStatusCue::AWizardCombatStatusCue()
{
	GameplayCueTag = WizardCombat::WindupTag();
	bAutoDestroyOnRemove = true;
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
	RingMesh = Plane.Object;
}
bool AWizardCombatStatusCue::OnActive_Implementation(AActor* Target, const FGameplayCueParameters& P)
{
	// 减速由GE触发，其来源保存在EffectContext；蓄力则直接传入SourceObject。
	const auto* Profile = WizardCombat::ResolvePresentation(P.GetSourceObject());
	if (P.RawMagnitude > 0 && Profile && Profile->TargetRingMaterial && Target)
	{
		if (!TargetRing)
		{
			TargetRing = NewObject<UStaticMeshComponent>(this);
			TargetRing->SetStaticMesh(RingMesh);
			TargetRing->SetMaterial(0, Profile->TargetRingMaterial);
			TargetRing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			TargetRing->SetCastShadow(false);
			TargetRing->RegisterComponent();
		}
		TargetRing->SetWorldLocation(P.Location + FVector(0, 0, 3));
		// 引擎Plane边长100cm；材质外圆半径0.5，直径严格对应两倍伤害半径。
		TargetRing->SetWorldScale3D(FVector(P.RawMagnitude / 50.f));
		return true;
	}
	if (!Effect)
		Effect = WizardCombat::SpawnStyle(
		    Target, Profile, GameplayCueTag == WizardCombat::SlowTag() ? EWizardCombatStage::Status : EWizardCombatStage::Windup,
		    P.Location, true);

	return true;
}
bool AWizardCombatStatusCue::OnRemove_Implementation(AActor* Target, const FGameplayCueParameters& P)
{
	if (Effect)
	{
		Effect->DestroyComponent();
		Effect = nullptr;
	}
	if (TargetRing)
	{
		TargetRing->DestroyComponent();
		TargetRing = nullptr;
	}
	return true;
}
void AWizardCombatStatusCue::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Effect)
	{
		Effect->DestroyComponent();
		Effect = nullptr;
	}
	if (TargetRing)
	{
		TargetRing->DestroyComponent();
		TargetRing = nullptr;
	}
	Super::EndPlay(Reason);
}
