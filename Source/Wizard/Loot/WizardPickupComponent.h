// Wizard 项目：玩家负责候选检测、接收权限及跨背包/金币的拾取协调。
#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WizardPickupComponent.generated.h"
class APawn;
class AWizardWorldPickup;

UCLASS()
class WIZARD_API UWizardPickupComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UWizardPickupComponent();
	// 换角时解除旧角色事件；重复绑定不补发拾取，不清空玩家丢弃门控。
	void BindAvatar(APawn* Avatar);
	void BlockUntilExit(AWizardWorldPickup* Pickup);
protected:
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	// 只由BeginOverlap调用一次，失败或部分接收后不自动重试。
	bool TryCollect(AWizardWorldPickup* Pickup);
	UFUNCTION() void HandleOverlapBegin(AActor* Actor, AActor* Other);
	UFUNCTION() void HandleOverlapEnd(AActor* Actor, AActor* Other);
	UFUNCTION() void HandleAvatarEnd(AActor* Actor, EEndPlayReason::Type Reason);
	TWeakObjectPtr<APawn> BoundAvatar;
	TSet<TWeakObjectPtr<AWizardWorldPickup>> BlockedDrops;
	bool bCollecting = false;
};
