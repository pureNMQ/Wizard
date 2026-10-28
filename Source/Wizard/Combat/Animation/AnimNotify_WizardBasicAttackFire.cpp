#include "Combat/Animation/AnimNotify_WizardBasicAttackFire.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GAS/WizardCombatTags.h"

UAnimNotify_WizardBasicAttackFire::UAnimNotify_WizardBasicAttackFire()
{
	// 出手与蒙太奇推进同步，取消后不保留已排队的普通 Notify。
	bIsNativeBranchingPoint = true;
#if WITH_EDITORONLY_DATA
	bShouldFireInEditor = false;
#endif
}

void UAnimNotify_WizardBasicAttackFire::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AActor* Avatar = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!IsValid(Avatar) || !Avatar->HasAuthority() || !MeshComp->GetWorld() || !MeshComp->GetWorld()->IsGameWorld()) { return; }
	FGameplayEventData Payload;
	Payload.EventTag = WizardCombat::BasicAttackFireTag();
	Payload.Instigator = Avatar;
	Payload.Target = Avatar;
	Payload.OptionalObject = Animation;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Avatar, Payload.EventTag, Payload);
}
