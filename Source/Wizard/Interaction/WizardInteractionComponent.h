// Wizard 项目：持有当前玩家的交互候选与提示，全部由事件驱动，不使用Tick/定时扫描。
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WizardInteractionComponent.generated.h"
class UWizardInteractionTarget;
class USceneComponent;
class APawn;
DECLARE_MULTICAST_DELEGATE_OneParam(FWizardInteractionPromptChanged, const FText&);

UCLASS()
class WIZARD_API UWizardInteractionComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UWizardInteractionComponent();
	void BindAvatar(APawn* Avatar);
	void RegisterTarget(UWizardInteractionTarget* Target);
	void UnregisterTarget(UWizardInteractionTarget* Target);
	void Refresh();
	void SetSuppressed(bool bInSuppressed);
	UFUNCTION(BlueprintCallable) bool TryInteract();
	FText GetPrompt() const { return CurrentPrompt; }
	UWizardInteractionTarget* GetCurrentTarget() const { return CurrentTarget.Get(); }
	// 菜单提交的权限查询，不受提示隐藏影响；只检查已注册候选，不扫描场景。
	UWizardInteractionTarget* FindAvailableTarget(TSubclassOf<AActor> OwnerType) const;
	FWizardInteractionPromptChanged OnPromptChanged;
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	void OnAvatarMoved(USceneComponent* Component, EUpdateTransformFlags Flags, ETeleportType Teleport);
	UFUNCTION() void OnAvatarEnded(AActor* Actor, EEndPlayReason::Type Reason);
	TWeakObjectPtr<APawn> BoundAvatar;
	TArray<TWeakObjectPtr<UWizardInteractionTarget>> Candidates;
	TWeakObjectPtr<UWizardInteractionTarget> CurrentTarget;
	FDelegateHandle AvatarMoveHandle;
	FText CurrentPrompt;
	FVector LastAvatarLocation = FVector::ZeroVector;
	bool bSuppressed = false;
	bool bRefreshing = false;
};
