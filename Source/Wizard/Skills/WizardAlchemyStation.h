// Wizard 项目：现有Alchemy的交互接入，不替换或移动场景模型。
#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WizardAlchemyStation.generated.h"
class UBoxComponent;
class AWizardPlayerState;
class UWizardInteractionTarget;
UCLASS(Blueprintable)
class WIZARD_API AWizardAlchemyStation : public AActor
{
	GENERATED_BODY()
public:
	AWizardAlchemyStation();
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly) TObjectPtr<AActor> AlchemyActor;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(ClampMin="1")) float InteractionDistance = 250.f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UBoxComponent> CampBounds;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UWizardInteractionTarget> InteractionTarget;
	void InitializeInteraction();
	bool CanInteract(const AWizardPlayerState* State) const;
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	bool IsInCamp(const AWizardPlayerState* State) const;
	bool OpenSkills(AWizardPlayerState* State);
	void OnCampMoved(USceneComponent* Component, EUpdateTransformFlags Flags, ETeleportType Teleport);
	FDelegateHandle CampMoveHandle;
};
