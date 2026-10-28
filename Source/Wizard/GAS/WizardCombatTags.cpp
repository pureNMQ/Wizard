#include "GAS/WizardCombatTags.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_WizardCooldownBasicAttack, "Cooldown.Wizard.BasicAttack");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_WizardEventBasicAttackFire, "Event.Wizard.BasicAttack.Fire");

FGameplayTag WizardCombat::BasicAttackCooldownTag()
{
	return TAG_WizardCooldownBasicAttack;
}

FGameplayTag WizardCombat::BasicAttackFireTag()
{
	return TAG_WizardEventBasicAttackFire;
}
