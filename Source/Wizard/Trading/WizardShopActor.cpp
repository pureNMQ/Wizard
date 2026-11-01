#include "Trading/WizardShopActor.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "GAS/WizardAttributeSet.h"
#include "GameFramework/Pawn.h"
#include "Interaction/WizardInteractionTarget.h"
#include "Trading/WizardShopCatalog.h"
#include "UI/WizardShopWidgets.h"
#include "UI/WizardUIRoot.h"
#include "WizardPlayerController.h"
#include "WizardPlayerState.h"

AWizardShopActor::AWizardShopActor()
{
	PrimaryActorTick.bCanEverTick = false;
	CampBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("CampBounds"));
	SetRootComponent(CampBounds);
	CampBounds->SetBoxExtent(FVector(1500.f, 1500.f, 500.f));
	CampBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	InteractionTarget = CreateDefaultSubobject<UWizardInteractionTarget>(TEXT("InteractionTarget"));
	InteractionTarget->SetupAttachment(CampBounds);
}

void AWizardShopActor::BeginPlay()
{
	Super::BeginPlay();
	InitializeInteraction();
	CampMoveHandle = CampBounds->TransformUpdated.AddUObject(this, &ThisClass::OnCampMoved);
}

void AWizardShopActor::InitializeInteraction()
{
	InteractionTarget->Condition.BindUObject(this, &ThisClass::IsInCamp);
	InteractionTarget->Action.BindUObject(this, &ThisClass::OpenShop);
	InteractionTarget->InitializeTarget(this, InteractionDistance, FText::FromString(TEXT("E　商店")));
}

bool AWizardShopActor::CanTrade(const AWizardPlayerState* State) const
{
	return IsValid(Catalog) && ShopMenuClass && InteractionTarget->IsAvailable(State);
}

bool AWizardShopActor::IsInCamp(const AWizardPlayerState* State) const
{
	const APawn* Avatar = State ? Cast<APawn>(State->GetAbilitySystemComponent()->GetAvatarActor()) : nullptr;
	if (!IsValid(Catalog) || !Catalog->IsConfigurationValid() || !ShopMenuClass || !IsValid(Avatar) || State->IsInChallenge() || State->GetAttributes()->GetHealth() <= 0.f) return false;
	const FVector Local = CampBounds->GetComponentTransform().InverseTransformPosition(Avatar->GetActorLocation());
	const FVector Extent = CampBounds->GetUnscaledBoxExtent();
	return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
}

bool AWizardShopActor::OpenShop(AWizardPlayerState* State)
{
	AWizardPlayerController* Controller = State ? Cast<AWizardPlayerController>(State->GetOwner()) : nullptr;
	UWizardUIRoot* Root = Controller ? Controller->GetUIRoot() : nullptr;
	return Root && Root->OpenShop(this);
}

void AWizardShopActor::OnCampMoved(USceneComponent* Component, EUpdateTransformFlags Flags, ETeleportType Teleport)
{
	InteractionTarget->NotifyAvailabilityChanged();
}

void AWizardShopActor::EndPlay(const EEndPlayReason::Type Reason)
{
	CampBounds->TransformUpdated.Remove(CampMoveHandle);
	Super::EndPlay(Reason);
}
