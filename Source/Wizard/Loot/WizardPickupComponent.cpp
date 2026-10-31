#include "Loot/WizardPickupComponent.h"
#include "Loot/WizardWorldPickup.h"
#include "Inventory/WizardInventoryComponent.h"
#include "WizardPlayerState.h"
#include "AbilitySystemComponent.h"
#include "GAS/WizardAttributeSet.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UWizardPickupComponent::UWizardPickupComponent() { PrimaryComponentTick.bCanEverTick = false; }

void UWizardPickupComponent::BindAvatar(APawn* Avatar)
{
	if (Avatar && BoundAvatar.Get() == Avatar) return;
	if (BoundAvatar.IsValid())
	{
		BoundAvatar->OnActorBeginOverlap.RemoveDynamic(this, &ThisClass::HandleOverlapBegin);
		BoundAvatar->OnActorEndOverlap.RemoveDynamic(this, &ThisClass::HandleOverlapEnd);
		BoundAvatar->OnEndPlay.RemoveDynamic(this, &ThisClass::HandleAvatarEnd);
	}
	BoundAvatar = Avatar;
	if (!IsValid(Avatar) || !GetOwner()->HasAuthority()) return;
	Avatar->OnActorBeginOverlap.AddUniqueDynamic(this, &ThisClass::HandleOverlapBegin);
	Avatar->OnActorEndOverlap.AddUniqueDynamic(this, &ThisClass::HandleOverlapEnd);
	Avatar->OnEndPlay.AddUniqueDynamic(this, &ThisClass::HandleAvatarEnd);

}

void UWizardPickupComponent::BlockUntilExit(AWizardWorldPickup* Pickup)
{
	if (IsValid(Pickup)) BlockedDrops.Add(Pickup);
}

void UWizardPickupComponent::HandleOverlapEnd(AActor* Actor, AActor* Other)
{
	// 整个角色离开时才解除门控；再次BeginOverlap才会触发下一次接收。
	if (Actor == BoundAvatar.Get() && !Actor->IsActorBeingDestroyed() && !Actor->IsOverlappingActor(Other))
	{
		if (AWizardWorldPickup* Pickup = Cast<AWizardWorldPickup>(Other)) BlockedDrops.Remove(Pickup);
	}
}

void UWizardPickupComponent::HandleOverlapBegin(AActor* Actor, AActor* Other)
{
	if (Actor == BoundAvatar.Get()) TryCollect(Cast<AWizardWorldPickup>(Other));
}

bool UWizardPickupComponent::TryCollect(AWizardWorldPickup* Pickup)
{
	AWizardPlayerState* Player = Cast<AWizardPlayerState>(GetOwner());
	APawn* Avatar = BoundAvatar.Get();
	if (bCollecting || !Player || !Player->HasAuthority() || !IsValid(Avatar) || !IsValid(Pickup)
		|| BlockedDrops.Contains(Pickup) || !Avatar->IsOverlappingActor(Pickup)
		|| Player->GetAbilitySystemComponent()->GetAvatarActor() != Avatar || Player->GetAttributes()->GetHealth() <= 0.f
		|| GetWorld()->IsPaused()) return false;
	TGuardValue<bool> Guard(bCollecting, true);
	return Pickup->TransferTo([&](int32 Offered)
	{
		if (Pickup->IsGold()) return Player->TryAddGold(Offered) ? Offered : 0;
		int32 Accepted = 0;
		Player->GetInventory()->AddItems(Pickup->GetDefinition(), Offered, Accepted);
		return Accepted;
	});
}

void UWizardPickupComponent::HandleAvatarEnd(AActor* Actor, EEndPlayReason::Type Reason) { BindAvatar(nullptr); }

void UWizardPickupComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	BindAvatar(nullptr);
	BlockedDrops.Reset();
	Super::EndPlay(Reason);
}
