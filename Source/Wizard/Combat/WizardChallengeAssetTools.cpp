#include "Combat/WizardChallengeAssetTools.h"
#if WITH_EDITOR
#include "Combat/WizardEnemy.h"
#include "Combat/WizardEnemyConfig.h"
#include "Challenge/WizardChallengeArena.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Blueprint.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#endif

bool UWizardChallengeAssetTools::BuildEnemyDefinitions()
{
#if WITH_EDITOR
	auto Save = [](UObject* Asset)
	{
		Asset->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		return UPackage::SavePackage(Asset->GetOutermost(), Asset, *FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
	};
	for (const FString Name : { FString(TEXT("Slime")), FString(TEXT("Beholder")) })
	{
		const FString ConfigPath = TEXT("/Game/Wizard/Enemies/DA_") + Name;
		UWizardEnemyConfig* Config = LoadObject<UWizardEnemyConfig>(nullptr, *ConfigPath, nullptr, LOAD_NoWarn);
		if (!Config)
		{
			Config = NewObject<UWizardEnemyConfig>(CreatePackage(*ConfigPath), *FPackageName::GetLongPackageAssetName(ConfigPath), RF_Public | RF_Standalone);
			FAssetRegistryModule::AssetCreated(Config);
		}
		Config->BehaviorTree = LoadObject<UBehaviorTree>(nullptr, TEXT("/Game/Wizard/AI/BT_Enemy"));
		Config->SkeletalMesh = LoadObject<USkeletalMesh>(nullptr, *(TEXT("/Game/MonsterForSurvivalGame/Mesh/Polyart/") + Name + TEXT("_SK")));
		Config->AnimClass = LoadClass<UAnimInstance>(nullptr, *(TEXT("/Game/Wizard/Enemies/Animation/ABP_") + Name + TEXT(".ABP_") + Name + TEXT("_C")));
		Config->AttackMontage = LoadObject<UAnimMontage>(nullptr, *(TEXT("/Game/Wizard/Enemies/Animation/AM_") + Name + TEXT("Attack")));
		Config->DeathAnimation = LoadObject<UAnimSequence>(nullptr, *(TEXT("/Game/MonsterForSurvivalGame/Animation/Polyart/") + Name + TEXT("/") + Name + TEXT("_Die_ANIM")));
		Config->bRanged = Name == TEXT("Beholder");
		Config->Health = Config->bRanged ? 70.f : 50.f;
		Config->AttackRange = Config->bRanged ? 650.f : 160.f;
		Config->MoveSpeed = Config->bRanged ? 190.f : 240.f;
		Config->AttackInterval = Config->bRanged ? 2.f : 1.5f;
		Config->AttackWindup = Config->AttackMontage ? Config->AttackMontage->GetPlayLength() * .4f : .25f;
		if (!Config->SkeletalMesh || !Config->AnimClass || !Config->AttackMontage || !Config->DeathAnimation || !Config->BehaviorTree || !Save(Config)) return false;
		const FString BPPath = TEXT("/Game/Wizard/Enemies/BP_") + Name;
		UBlueprint* BP = LoadObject<UBlueprint>(nullptr, *BPPath, nullptr, LOAD_NoWarn);
		if (!BP)
		{
			BP = FKismetEditorUtilities::CreateBlueprint(AWizardEnemy::StaticClass(), CreatePackage(*BPPath), *FPackageName::GetLongPackageAssetName(BPPath), BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
			FAssetRegistryModule::AssetCreated(BP);
		}
		FKismetEditorUtilities::CompileBlueprint(BP);
		CastChecked<AWizardEnemy>(BP->GeneratedClass->GetDefaultObject())->EnemyConfig = Config;
		CastChecked<AWizardEnemy>(BP->GeneratedClass->GetDefaultObject())->EnemyHealthBar->SetWidgetClass(LoadClass<UUserWidget>(nullptr, TEXT("/Game/Wizard/UI/WBP_EnemyHealthBar.WBP_EnemyHealthBar_C")));
		FBlueprintEditorUtils::MarkBlueprintAsModified(BP);
		if (!Save(BP)) return false;
	}
	return true;
#else
	return false;
#endif
}

bool UWizardChallengeAssetTools::ConfigureChallengeMap()
{
#if WITH_EDITOR
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World || World->GetOutermost()->GetName() != TEXT("/Game/Wizard/Maps/L_WizardClearing")) return false;
	AWizardChallengeArena* Arena = nullptr;
	for (TActorIterator<AWizardChallengeArena> It(World); It; ++It) { Arena = *It; break; }
	if (!Arena)
	{
		Arena = World->SpawnActor<AWizardChallengeArena>();
		Arena->SetActorLabel(TEXT("Challenge_Arena"));
		Arena->SetActorLocation(FVector(2200, 0, 0));
	}
	Arena->Modify();
	Arena->ArenaRadius = 1780.f;
	Arena->EntryInset = 140.f;
	Arena->CampReturnLocation = FVector(-2100, -100, 115);
	Arena->SpawnOffsets = { FVector(100, 1250, 100), FVector(1200,700,100), FVector(100,-1250,100), FVector(700,-700,100), FVector(-600,700,100) };
	TSubclassOf<AWizardEnemy> Slime = LoadClass<AWizardEnemy>(nullptr, TEXT("/Game/Wizard/Enemies/BP_Slime.BP_Slime_C"));
	TSubclassOf<AWizardEnemy> Beholder = LoadClass<AWizardEnemy>(nullptr, TEXT("/Game/Wizard/Enemies/BP_Beholder.BP_Beholder_C"));
	if (!Slime || !Beholder) return false;
	Arena->Waves.Empty();
	for (int32 Index = 0; Index < 3; ++Index)
	{
		FWizardChallengeWave Wave;
		FWizardChallengeSpawn Melee; Melee.EnemyClass = Slime; Melee.Count = Index + 1;
		Wave.Spawns.Add(Melee);
		if (Index > 0) { FWizardChallengeSpawn Ranged; Ranged.EnemyClass = Beholder; Ranged.Count = Index; Wave.Spawns.Add(Ranged); }
		Arena->Waves.Add(Wave);
	}
	Arena->MarkPackageDirty();
	return Arena->IsConfigurationValid();
#else
	return false;
#endif
}
