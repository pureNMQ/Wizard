#include "Skills/AnimNotify_WizardSkillFire.h"
#include "Skills/WizardSkillEffects.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "Engine/World.h"
UAnimNotify_WizardSkillFire::UAnimNotify_WizardSkillFire()
{
	bIsNativeBranchingPoint = true;
#if WITH_EDITORONLY_DATA
	bShouldFireInEditor = false;
#endif
}
void UAnimNotify_WizardSkillFire::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	AActor* Avatar = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!Avatar || !Avatar->HasAuthority() || !Avatar->GetWorld()->IsGameWorld()) return;
	FGameplayEventData Event;
	Event.Instigator = Avatar;
	Event.Target = Avatar;
	Event.OptionalObject = Animation;
	Event.EventTag = WizardSkills::FireTag();
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Avatar, Event.EventTag, Event);
}
