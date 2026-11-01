// Wizard 项目：商人NPC的场景交互；模型和材质由其蓝图配置。
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WizardShopActor.generated.h"

class AWizardPlayerState;
class UBoxComponent;
class UWizardInteractionTarget;
class UWizardShopCatalog;
class UWizardShopMenu;

UCLASS(Blueprintable)
class WIZARD_API AWizardShopActor : public AActor
{
	GENERATED_BODY()
public:
	AWizardShopActor();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop") TObjectPtr<UWizardShopCatalog> Catalog;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop") TSubclassOf<UWizardShopMenu> ShopMenuClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Shop", meta=(ClampMin="1")) float InteractionDistance = 250.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Shop") TObjectPtr<UBoxComponent> CampBounds;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Shop") TObjectPtr<UWizardInteractionTarget> InteractionTarget;
	void InitializeInteraction();
	UWizardShopCatalog* GetCatalog() const { return Catalog; }
	// 打开与提交交易均重新检查；菜单隐藏交互提示不撤销这项业务权限。
	bool CanTrade(const AWizardPlayerState* State) const;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	bool IsInCamp(const AWizardPlayerState* State) const;
	bool OpenShop(AWizardPlayerState* State);
	void OnCampMoved(USceneComponent* Component, EUpdateTransformFlags Flags, ETeleportType Teleport);
	FDelegateHandle CampMoveHandle;
};
