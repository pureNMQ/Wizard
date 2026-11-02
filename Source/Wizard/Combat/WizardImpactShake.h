#pragma once
#include "CoreMinimal.h"
#include "Camera/CameraShakeBase.h"
#include "WizardImpactShake.generated.h"
UCLASS()
class UWizardImpactShakePattern : public UCameraShakePattern
{
	GENERATED_BODY()
  public:
	float Duration = .08f;
	float Strength = .15f;

  protected:
	virtual void StartShakePatternImpl(const FCameraShakePatternStartParams& Params) override
	{
		Elapsed = 0;
		Started = FPlatformTime::Seconds();
	}
	virtual void UpdateShakePatternImpl(const FCameraShakePatternUpdateParams& Params, FCameraShakePatternUpdateResult& Result) override;
	virtual void StopShakePatternImpl(const FCameraShakePatternStopParams& Params) override
	{
		Elapsed = Duration;
	}
	virtual bool IsFinishedImpl() const override
	{
		return Elapsed >= Duration;
	}

  private:
	float Elapsed = 0;
	double Started = 0;
};
UCLASS()
class UWizardLightImpactShake : public UCameraShakeBase
{
	GENERATED_BODY()
  public:
	UWizardLightImpactShake(const FObjectInitializer& ObjectInitializer);
};
UCLASS()
class UWizardMediumImpactShake : public UCameraShakeBase
{
	GENERATED_BODY()
  public:
	UWizardMediumImpactShake(const FObjectInitializer& ObjectInitializer);
};
UCLASS()
class UWizardHeavyImpactShake : public UCameraShakeBase
{
	GENERATED_BODY()
  public:
	UWizardHeavyImpactShake(const FObjectInitializer& ObjectInitializer);
};
