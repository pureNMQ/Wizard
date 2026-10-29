#include "Interaction/WizardInteractionTarget.h"
#include "Interaction/WizardInteractionComponent.h"
#include "WizardPlayerState.h"
#include "GAS/WizardAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "Components/MeshComponent.h"

namespace
{
	AWizardPlayerState* PlayerFor(AActor* Actor)
	{
		const APawn* Pawn = Cast<APawn>(Actor);
		return Pawn ? Pawn->GetPlayerState<AWizardPlayerState>() : nullptr;
	}
}
UWizardInteractionTarget::UWizardInteractionTarget()
{
	PrimaryComponentTick.bCanEverTick = false;
	InitSphereRadius(250.f);
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SetGenerateOverlapEvents(true);
	SetCanEverAffectNavigation(false);
	SetHiddenInGame(true);
}
void UWizardInteractionTarget::BeginPlay()
{
	Super::BeginPlay();
	OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnRangeEntered);
	OnComponentEndOverlap.AddDynamic(this, &ThisClass::OnRangeLeft);
}
void UWizardInteractionTarget::DetachFocus()
{
	if (IsValid(FocusActor))
	{
		FocusActor->OnEndPlay.RemoveDynamic(this, &ThisClass::OnFocusEnded);
		if (FocusActor->GetRootComponent()) FocusActor->GetRootComponent()->TransformUpdated.Remove(FocusMoveHandle);
	}
	FocusMoveHandle.Reset();
}
void UWizardInteractionTarget::InitializeTarget(AActor* InFocusActor, float InDistance, const FText& InPrompt)
{
	DetachFocus();
	FocusActor = InFocusActor;
	Distance = InDistance;
	Prompt = InPrompt;
	bInteractionEnabled = IsValid(FocusActor) && FMath::IsFinite(Distance) && Distance > 0.f;
	if (bInteractionEnabled && FocusActor->GetRootComponent())
	{
		AttachToComponent(FocusActor->GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		SetAbsolute(false, false, true);
		SetWorldScale3D(FVector::OneVector);
		SetSphereRadius(Distance, true);
		FocusMoveHandle = FocusActor->GetRootComponent()->TransformUpdated.AddUObject(this, &ThisClass::OnFocusMoved);
		FocusActor->OnEndPlay.AddDynamic(this, &ThisClass::OnFocusEnded);
	}
	UpdateOverlaps();
	TArray<AActor*> Overlapping;
	GetOverlappingActors(Overlapping, APawn::StaticClass());
	for (AActor* Actor : Overlapping) if (AWizardPlayerState* State = PlayerFor(Actor)) State->GetInteraction()->RegisterTarget(this);
	NotifyAvailabilityChanged();
}
float UWizardInteractionTarget::GetDistanceSquared(const AWizardPlayerState* State) const
{
	const AActor* Avatar = State ? State->GetAbilitySystemComponent()->GetAvatarActor() : nullptr;
	return Avatar && IsValid(FocusActor) ? FVector::DistSquared2D(Avatar->GetActorLocation(), FocusActor->GetActorLocation()) : TNumericLimits<float>::Max();
}
FBox UWizardInteractionTarget::GetPromptWorldBounds() const
{
	if (!IsValid(FocusActor)) return FBox(GetComponentLocation(), GetComponentLocation());
	FBox ModelBounds(ForceInit);
	TArray<UMeshComponent*> Meshes;
	FocusActor->GetComponents(Meshes);
	for (const UMeshComponent* Mesh : Meshes)
	{
		if (Mesh->IsRegistered() && Mesh->IsVisible() && !Mesh->bHiddenInGame) ModelBounds += Mesh->Bounds.GetBox();
	}
	if (!ModelBounds.IsValid) ModelBounds = FBox(FocusActor->GetActorLocation(), FocusActor->GetActorLocation());
	return ModelBounds.ShiftBy(FVector(0.f, 0.f, PromptHeightOffset));
}
bool UWizardInteractionTarget::IsAvailable(const AWizardPlayerState* State) const
{
	const AActor* Avatar = State ? State->GetAbilitySystemComponent()->GetAvatarActor() : nullptr;
	if (!bInteractionEnabled || !IsValid(FocusActor) || !IsValid(Avatar) || State->GetAttributes()->GetHealth() <= 0.f || !Action.IsBound()) return false;
	if (GetDistanceSquared(State) > FMath::Square(Distance)) return false;
	if (Condition.IsBound() && !Condition.Execute(State)) return false;
	FVector Center, Extent;
	FocusActor->GetActorBounds(false, Center, Extent);
	FCollisionQueryParams Query(SCENE_QUERY_STAT(WizardInteraction), false, Avatar);
	Query.AddIgnoredActor(GetOwner());
	FHitResult Hit;
	return !GetWorld()->LineTraceSingleByChannel(Hit, Avatar->GetActorLocation(), Center, ECC_Visibility, Query) || Hit.GetActor() == FocusActor;
}
bool UWizardInteractionTarget::Execute(AWizardPlayerState* State)
{
	return IsAvailable(State) && Action.Execute(State);
}
void UWizardInteractionTarget::NotifyAvailabilityChanged() { OnAvailabilityChanged.Broadcast(); }
void UWizardInteractionTarget::SetInteractionEnabled(bool bInEnabled) { bInteractionEnabled = bInEnabled; NotifyAvailabilityChanged(); }
void UWizardInteractionTarget::OnFocusMoved(USceneComponent* Component, EUpdateTransformFlags Flags, ETeleportType Teleport) { NotifyAvailabilityChanged(); }
void UWizardInteractionTarget::OnFocusEnded(AActor* Actor, EEndPlayReason::Type Reason) { SetInteractionEnabled(false); }
void UWizardInteractionTarget::OnRangeEntered(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent, int32 BodyIndex, bool bFromSweep, const FHitResult& Hit)
{
	if (AWizardPlayerState* State = PlayerFor(Other)) State->GetInteraction()->RegisterTarget(this);
}
void UWizardInteractionTarget::OnRangeLeft(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent, int32 BodyIndex)
{
	if (AWizardPlayerState* State = PlayerFor(Other))
	{
		if (!IsOverlappingActor(Other)) State->GetInteraction()->UnregisterTarget(this);
	}
}
void UWizardInteractionTarget::EndPlay(const EEndPlayReason::Type Reason)
{
	SetInteractionEnabled(false);
	DetachFocus();
	OnComponentBeginOverlap.RemoveAll(this);
	OnComponentEndOverlap.RemoveAll(this);
	OnAvailabilityChanged.Clear();
	Action.Unbind();
	Condition.Unbind();
	Super::EndPlay(Reason);
}
