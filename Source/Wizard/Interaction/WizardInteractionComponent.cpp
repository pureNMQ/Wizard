#include "Interaction/WizardInteractionComponent.h"
#include "Interaction/WizardInteractionTarget.h"
#include "WizardPlayerState.h"
#include "AbilitySystemComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UWizardInteractionComponent::UWizardInteractionComponent() { PrimaryComponentTick.bCanEverTick = false; }
void UWizardInteractionComponent::BindAvatar(APawn* Avatar)
{
	if (Avatar && BoundAvatar.Get() == Avatar) { Refresh(); return; }
	if (BoundAvatar.IsValid())
	{
		BoundAvatar->OnEndPlay.RemoveDynamic(this, &ThisClass::OnAvatarEnded);
		if (BoundAvatar->GetRootComponent()) BoundAvatar->GetRootComponent()->TransformUpdated.Remove(AvatarMoveHandle);
	}
	for (const auto& Candidate : Candidates) if (Candidate.IsValid()) Candidate->OnAvailabilityChanged.RemoveAll(this);
	Candidates.Reset();
	BoundAvatar = Avatar;
	if (IsValid(Avatar))
	{
		LastAvatarLocation = Avatar->GetActorLocation();
		AvatarMoveHandle = Avatar->GetRootComponent()->TransformUpdated.AddUObject(this, &ThisClass::OnAvatarMoved);
		Avatar->OnEndPlay.AddDynamic(this, &ThisClass::OnAvatarEnded);
		// 占有/重生时仅对当前Pawn已有重叠做一次初始化，不枚举整个场景。
		TArray<UPrimitiveComponent*> Overlaps;
		Avatar->GetOverlappingComponents(Overlaps);
		for (UPrimitiveComponent* Overlap : Overlaps) if (auto* Target = Cast<UWizardInteractionTarget>(Overlap)) RegisterTarget(Target);
	}
	Refresh();
}
void UWizardInteractionComponent::RegisterTarget(UWizardInteractionTarget* Target)
{
	if (!IsValid(Target) || Candidates.Contains(Target)) return;
	Candidates.Add(Target);
	Target->OnAvailabilityChanged.AddUObject(this, &ThisClass::Refresh);
	Refresh();
}
void UWizardInteractionComponent::UnregisterTarget(UWizardInteractionTarget* Target)
{
	if (Target) Target->OnAvailabilityChanged.RemoveAll(this);
	Candidates.Remove(Target);
	Refresh();
}
UWizardInteractionTarget* UWizardInteractionComponent::FindAvailableTarget(TSubclassOf<AActor> OwnerType) const
{
	const AWizardPlayerState* State = Cast<AWizardPlayerState>(GetOwner());
	if (!BoundAvatar.IsValid() || !State || State->GetAbilitySystemComponent()->GetAvatarActor() != BoundAvatar.Get()) return nullptr;
	UWizardInteractionTarget* Best = nullptr;
	float BestDistance = TNumericLimits<float>::Max();
	for (const auto& Candidate : Candidates)
	{
		UWizardInteractionTarget* Target = Candidate.Get();
		if (!IsValid(Target) || (OwnerType && !Target->GetOwner()->IsA(OwnerType)) || !Target->IsAvailable(State)) continue;
		const float Distance = Target->GetDistanceSquared(State);
		if (Distance < BestDistance || (Distance == BestDistance && Best && Target->GetPathName() < Best->GetPathName())) { Best = Target; BestDistance = Distance; }
	}
	return Best;
}
void UWizardInteractionComponent::Refresh()
{
	if (bRefreshing) return;
	TGuardValue<bool> Guard(bRefreshing, true);
	Candidates.RemoveAll([](const auto& Target) { return !Target.IsValid(); });
	UWizardInteractionTarget* Target = bSuppressed ? nullptr : FindAvailableTarget(nullptr);
	const FText Prompt = Target ? Target->GetPrompt() : FText::GetEmpty();
	const bool bChanged = Target != CurrentTarget.Get() || !Prompt.EqualTo(CurrentPrompt);
	CurrentTarget = Target;
	CurrentPrompt = Prompt;
	if (bChanged) OnPromptChanged.Broadcast(CurrentPrompt);
}
void UWizardInteractionComponent::SetSuppressed(bool bInSuppressed) { bSuppressed = bInSuppressed; Refresh(); }
bool UWizardInteractionComponent::TryInteract()
{
	Refresh();
	UWizardInteractionTarget* Target = CurrentTarget.Get();
	AWizardPlayerState* State = Cast<AWizardPlayerState>(GetOwner());
	if (bSuppressed || !Target || !GetWorld() || GetWorld()->IsPaused()) return false;
	const bool bSucceeded = Target->Execute(State);
	Refresh();
	return bSucceeded;
}
void UWizardInteractionComponent::OnAvatarMoved(USceneComponent* Component, EUpdateTransformFlags Flags, ETeleportType Teleport)
{
	if (!BoundAvatar.IsValid()) return;
	const FVector Location = BoundAvatar->GetActorLocation();
	if (!Location.Equals(LastAvatarLocation)) { LastAvatarLocation = Location; Refresh(); }
}
void UWizardInteractionComponent::OnAvatarEnded(AActor* Actor, EEndPlayReason::Type Reason) { BindAvatar(nullptr); }
void UWizardInteractionComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	BindAvatar(nullptr);
	OnPromptChanged.Clear();
	Super::EndPlay(Reason);
}
