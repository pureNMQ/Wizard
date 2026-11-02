#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Blueprint/UserWidget.h"
#include "Combat/WizardCombatPresentation.h"
#include "WizardCombatFeedbackComponent.generated.h"
struct FWizardFloatingNumber
{
	TWeakObjectPtr<AActor> Target;
	FVector Position;
	float Amount = 0;
	double Born = 0;
	int32 Kind = 0;
};
// 玩家本地反馈生命周期；只在这里持有全局时间倍率修改。
UCLASS(ClassGroup = Combat)
class WIZARD_API UWizardCombatFeedbackComponent : public UActorComponent
{
	GENERATED_BODY()
  public:
	UWizardCombatFeedbackComponent();
	UPROPERTY(EditAnywhere, Category = "Feedback") bool bHitStopEnabled = true;
	UPROPERTY(EditAnywhere, Category = "Feedback") bool bShakeEnabled = true;
	UPROPERTY(EditAnywhere, Category = "Feedback", meta = (ClampMin = "0", ClampMax = "2")) float Intensity = 1;
	UPROPERTY(EditAnywhere, Category = "Feedback") TSubclassOf<class UWizardCombatFeedbackWidget> WidgetClass;
	void Consume(const FWizardCombatResult& Result);
	void ResetFeedback();
	void RequestImpact(float StopSeconds, int32 Level);
	const TArray<FWizardFloatingNumber>& GetNumbers() const
	{
		return Numbers;
	}
	float GetHurtOpacity() const
	{
		return HurtOpacity;
	}
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

  protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

  private:
	void AddNumber(const FWizardCombatResult& R, float Amount, int32 Kind);
	void RestoreTime();
	TArray<FWizardFloatingNumber> Numbers;
	FDelegateHandle DeactivateHandle, ActivateHandle;
	bool bApplicationActive = true;
	void OnDeactivate();
	void OnActivate();
	float PendingStop = 0, OriginalDilation = 1, HurtOpacity = 0;
	int32 PendingLevel = 0;
	double StopEnd = 0, NextImpact = 0, LastRealTime = 0;
	bool bOwnsDilation = false;
	UPROPERTY() TObjectPtr<class UCameraShakeBase> ActiveShake;
	UPROPERTY() TObjectPtr<UWizardCombatFeedbackWidget> Widget;
};
// 无输入的HUD绘制模块；布局/字体/颜色可由Widget Blueprint配置。
UCLASS(Blueprintable)
class WIZARD_API UWizardCombatFeedbackWidget : public UUserWidget
{
	GENERATED_BODY()
  public:
	UPROPERTY(EditAnywhere, Category = "Feedback") int32 FontSize = 22;
	UPROPERTY(EditAnywhere, Category = "Feedback") FLinearColor DamageColor = FLinearColor(1, .85f, .35f);
	UPROPERTY(EditAnywhere, Category = "Feedback") FLinearColor PlayerDamageColor = FLinearColor(1, .15f, .12f);
	UPROPERTY(EditAnywhere, Category = "Feedback") FLinearColor ShieldColor = FLinearColor(.35f, .8f, 1);
	UPROPERTY(EditAnywhere, Category = "Feedback") FLinearColor HealColor = FLinearColor(.25f, 1, .45f);
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& CullingRect,
	                          FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool bParentEnabled) const override;
};
