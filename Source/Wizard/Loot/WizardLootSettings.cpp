#include "Loot/WizardLootSettings.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"

UTexture2D* UWizardLootSettings::ResolveIcon(const TSoftObjectPtr<UTexture2D>& Icon)
{
	if (UTexture2D* Texture = Icon.LoadSynchronous()) return Texture;
	return GEngine ? GEngine->DefaultTexture.Get() : nullptr;
}
