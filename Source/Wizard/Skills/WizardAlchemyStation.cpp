#include "Skills/WizardAlchemyStation.h"
#include "WizardPlayerState.h"
#include "GAS/WizardAttributeSet.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "AbilitySystemComponent.h"
#include "Interaction/WizardInteractionTarget.h"
#include "WizardPlayerController.h"
AWizardAlchemyStation::AWizardAlchemyStation()
{
	PrimaryActorTick.bCanEverTick = false;
	CampBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("CampBounds"));
	SetRootComponent(CampBounds);
	CampBounds->SetBoxExtent(FVector(1500.f, 1500.f, 500.f));
	CampBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionTarget = CreateDefaultSubobject<UWizardInteractionTarget>(TEXT("InteractionTarget"));
	InteractionTarget->SetupAttachment(CampBounds);
}
void AWizardAlchemyStation::BeginPlay()
{
	Super::BeginPlay();
	InitializeInteraction();
	CampMoveHandle = CampBounds->TransformUpdated.AddUObject(this, &ThisClass::OnCampMoved);
}
void AWizardAlchemyStation::InitializeInteraction()
{
	InteractionTarget->Condition.BindUObject(this, &ThisClass::IsInCamp);
	InteractionTarget->Action.BindUObject(this, &ThisClass::OpenSkills);
	InteractionTarget->InitializeTarget(AlchemyActor, InteractionDistance, FText::FromString(TEXT("E　技能搭配")));
}
bool AWizardAlchemyStation::CanInteract(const AWizardPlayerState* State) const { return InteractionTarget->IsAvailable(State); }
bool AWizardAlchemyStation::IsInCamp(const AWizardPlayerState* State) const
{
	const APawn* Avatar = State ? Cast<APawn>(State->GetAbilitySystemComponent()->GetAvatarActor()) : nullptr;
	if (!IsValid(AlchemyActor) || !Avatar || State->GetAttributes()->GetHealth() <= 0.f) return false;
	const FVector Local = CampBounds->GetComponentTransform().InverseTransformPosition(Avatar->GetActorLocation());
	const FVector Extent = CampBounds->GetUnscaledBoxExtent();
	if (FMath::Abs(Local.X) > Extent.X || FMath::Abs(Local.Y) > Extent.Y || FMath::Abs(Local.Z) > Extent.Z) return false;
	return true;
}
bool AWizardAlchemyStation::OpenSkills(AWizardPlayerState* State)
{
	AWizardPlayerController* Controller = State ? Cast<AWizardPlayerController>(State->GetOwner()) : nullptr;
	return Controller && Controller->OpenSkillMenu();
}
void AWizardAlchemyStation::OnCampMoved(USceneComponent* Component, EUpdateTransformFlags Flags, ETeleportType Teleport) { InteractionTarget->NotifyAvailabilityChanged(); }
void AWizardAlchemyStation::EndPlay(const EEndPlayReason::Type Reason)
{
	CampBounds->TransformUpdated.Remove(CampMoveHandle);
	Super::EndPlay(Reason);
}
