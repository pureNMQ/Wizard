#include "Combat/WizardImpactShake.h"
void UWizardImpactShakePattern::UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& P, FCameraShakePatternUpdateResult& R)
{
	Elapsed = static_cast<float>(FPlatformTime::Seconds() - Started);
	const float Fade = FMath::Clamp(1.f - Elapsed / Duration, 0.f, 1.f);
	R.Rotation.Pitch = FMath::Sin(Elapsed * 170.f) * Strength * Fade;
	R.Rotation.Yaw = FMath::Sin(Elapsed * 213.f) * Strength * Fade;
}
UWizardLightImpactShake::UWizardLightImpactShake(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bSingleInstance = true;
	SetRootShakePattern(CreateDefaultSubobject<UWizardImpactShakePattern>(TEXT("Impact")));
}
UWizardMediumImpactShake::UWizardMediumImpactShake(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bSingleInstance = true;
	auto* P = CreateDefaultSubobject<UWizardImpactShakePattern>(TEXT("Impact"));
	P->Duration = .12f;
	P->Strength = .3f;
	SetRootShakePattern(P);
}
UWizardHeavyImpactShake::UWizardHeavyImpactShake(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bSingleInstance = true;
	auto* P = CreateDefaultSubobject<UWizardImpactShakePattern>(TEXT("Impact"));
	P->Duration = .16f;
	P->Strength = .5f;
	SetRootShakePattern(P);
}
