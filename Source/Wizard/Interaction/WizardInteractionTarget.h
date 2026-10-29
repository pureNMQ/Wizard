// Wizard 项目：可复用交互目标；业务通过原生委托接入，范围与遮挡在此统一校验。
#pragma once
#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "WizardInteractionTarget.generated.h"
class AWizardPlayerState;
DECLARE_DELEGATE_RetVal_OneParam(bool, FWizardInteractionAction, AWizardPlayerState*);
DECLARE_DELEGATE_RetVal_OneParam(bool, FWizardInteractionCondition, const AWizardPlayerState*);
DECLARE_MULTICAST_DELEGATE(FWizardInteractionChanged);

UCLASS(ClassGroup=(Wizard), meta=(BlueprintSpawnableComponent))
class WIZARD_API UWizardInteractionTarget : public USphereComponent
{
	GENERATED_BODY()
public:
	UWizardInteractionTarget();
	// 在业务Actor BeginPlay中配置；不移动可视模型。回调用BindUObject，失效后自动拒绝交互。
	void InitializeTarget(AActor* InFocusActor, float InDistance, const FText& InPrompt);
	bool IsAvailable(const AWizardPlayerState* State) const;
	bool Execute(AWizardPlayerState* State);
	float GetDistanceSquared(const AWizardPlayerState* State) const;
	const FText& GetPrompt() const { return Prompt; }
	// 只读取可视模型包围盒并加高度偏移，供UI投影顶部；没有模型时回退到对象原点。
	FBox GetPromptWorldBounds() const;
	UPROPERTY(EditAnywhere, Category="Interaction") float PromptHeightOffset = 30.f;
	// 门、营地规则等外部条件发生变化时主动通知；按E还会重新校验。
	void NotifyAvailabilityChanged();
	void SetInteractionEnabled(bool bInEnabled);
	FWizardInteractionAction Action;
	FWizardInteractionCondition Condition;
	FWizardInteractionChanged OnAvailabilityChanged;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	void DetachFocus();
	void OnFocusMoved(USceneComponent* Component, EUpdateTransformFlags Flags, ETeleportType Teleport);
	UFUNCTION() void OnFocusEnded(AActor* Actor, EEndPlayReason::Type Reason);
	UFUNCTION() void OnRangeEntered(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent, int32 BodyIndex, bool bFromSweep, const FHitResult& Hit);
	UFUNCTION() void OnRangeLeft(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent, int32 BodyIndex);
	UPROPERTY(Transient) TObjectPtr<AActor> FocusActor;
	FDelegateHandle FocusMoveHandle;
	FText Prompt;
	float Distance = 250.f;
	bool bInteractionEnabled = false;
};
