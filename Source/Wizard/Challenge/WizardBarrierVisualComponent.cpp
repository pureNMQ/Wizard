#include "Challenge/WizardBarrierVisualComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

UWizardBarrierVisualComponent::UWizardBarrierVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCanEverAffectNavigation(false);
	SetCastShadow(false);
	bReceivesDecals = false;
	SetVisibility(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (Plane.Succeeded()) SetStaticMesh(Plane.Object);
}

void UWizardBarrierVisualComponent::BuildRing(float Radius)
{
	ClearInstances();
	if (!FMath::IsFinite(Radius) || Radius <= 0.f || !FMath::IsFinite(BarrierHeight) || BarrierHeight <= 0.f) return;
	if (!BarrierMaterial) BarrierMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Wizard/Materials/M_ArenaBarrier.M_ArenaBarrier"));
	if (!BarrierMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("结界视觉材质缺失，空气墙碰撞仍独立工作"));
		return;
	}
	UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BarrierMaterial, this);
	DynamicMaterial->SetScalarParameterValue(TEXT("BaseHeight"), GetComponentLocation().Z + 5.f);
	DynamicMaterial->SetScalarParameterValue(TEXT("Height"), BarrierHeight);
	SetMaterial(0, DynamicMaterial);
	constexpr int32 SegmentCount = 64;
	const float Width = 2.f * Radius * FMath::Tan(PI / SegmentCount) + 2.f;
	for (int32 Index = 0; Index < SegmentCount; ++Index)
	{
		const float Angle = 2.f * PI * Index / SegmentCount;
		const FVector Location(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, BarrierHeight * .5f + 5.f);
		// Plane 的局部X转为竖向，局部Y沿圆周切线；双面材质保证内外均可见。
		const FTransform Instance(FRotator(90.f, FMath::RadiansToDegrees(Angle), 0.f), Location, FVector(BarrierHeight / 100.f, Width / 100.f, 1.f));
		AddInstance(Instance);
	}
}
