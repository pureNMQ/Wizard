// Wizard 项目：结界表现独立于空气墙碰撞，仅在挑战期间显示。
#pragma once
#include "CoreMinimal.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "WizardBarrierVisualComponent.generated.h"

UCLASS(ClassGroup=(Wizard), meta=(BlueprintSpawnableComponent))
class WIZARD_API UWizardBarrierVisualComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()
public:
	UWizardBarrierVisualComponent();
	// 重建视觉实例，不改变任何碰撞或导航；无效半径保持隐藏。
	void BuildRing(float Radius);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Barrier") TObjectPtr<UMaterialInterface> BarrierMaterial;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Barrier", meta=(ClampMin="100")) float BarrierHeight = 350.f;
};
