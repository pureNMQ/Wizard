#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "Misc/AutomationTest.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "GameMapsSettings.h"
#include "Misc/PackageName.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "WizardCharacter.h"
#include "WizardGameMode.h"
#include "WizardPlayerController.h"
#include "WizardPlayerState.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardStartupConfigurationTest,
	"Wizard.Environment.StartupConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWizardStartupConfigurationTest::RunTest(const FString& Parameters)
{
	const UGameMapsSettings* Settings = GetDefault<UGameMapsSettings>();
	const FString ExpectedMap = TEXT("/Game/Wizard/Maps/L_WizardClearing");
	TestEqual(TEXT("Game default map preserves the agreed scene"),
		FPackageName::ObjectPathToPackageName(UGameMapsSettings::GetGameDefaultMap()), ExpectedMap);
	TestEqual(TEXT("Editor startup map preserves the agreed scene"),
		Settings->EditorStartupMap.GetLongPackageName(), ExpectedMap);
	TestTrue(TEXT("The startup map package exists"), FPackageName::DoesPackageExist(ExpectedMap));
	TestTrue(TEXT("Native GameMode leaves pawn selection at the engine default instead of referencing a character Blueprint"),
		GetDefault<AWizardGameMode>()->DefaultPawnClass == GetDefault<AGameModeBase>()->DefaultPawnClass);

	const FString ExpectedGameMode = TEXT("/Game/Wizard/Blueprints/BP_WizardGameMode.BP_WizardGameMode_C");
	TestEqual(TEXT("Project default selects the configurable GameMode Blueprint"),
		UGameMapsSettings::GetGlobalDefaultGameMode(), ExpectedGameMode);
	const UClass* GameModeClass = LoadClass<AGameModeBase>(nullptr, *UGameMapsSettings::GetGlobalDefaultGameMode());
	if (!TestNotNull(TEXT("Configured GameMode loads"), GameModeClass))
	{
		return false;
	}
	TestTrue(TEXT("Configured GameMode derives from WizardGameMode"), GameModeClass->IsChildOf(AWizardGameMode::StaticClass()));
	TestTrue(TEXT("Configured GameMode is a Blueprint class, not the native fallback"),
		GameModeClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint));
	const AGameModeBase* GameMode = GameModeClass->GetDefaultObject<AGameModeBase>();
	TestTrue(TEXT("The configured player pawn loads and derives from WizardCharacter"),
		GameMode->DefaultPawnClass && GameMode->DefaultPawnClass->IsChildOf(AWizardCharacter::StaticClass()));
	TestTrue(TEXT("The configured controller loads and derives from WizardPlayerController"),
		GameMode->PlayerControllerClass && GameMode->PlayerControllerClass->IsChildOf(AWizardPlayerController::StaticClass()));
	TestTrue(TEXT("The configured PlayerState retains Wizard GAS ownership"),
		GameMode->PlayerStateClass && GameMode->PlayerStateClass->IsChildOf(AWizardPlayerState::StaticClass()));
	TestTrue(TEXT("The player pawn is configured as a Blueprint class"),
		GameMode->DefaultPawnClass && GameMode->DefaultPawnClass->HasAnyClassFlags(CLASS_CompiledFromBlueprint));

	// 地图覆盖优先于项目设置，必须检查保存地图的实际选择，避免悄悄回退原生类。
	const UWorld* StartupWorld = LoadObject<UWorld>(nullptr, *UGameMapsSettings::GetGameDefaultMap());
	if (!TestNotNull(TEXT("Saved startup world loads for GameMode validation"), StartupWorld))
	{
		return false;
	}
	const AWorldSettings* WorldSettings = StartupWorld->GetWorldSettings();
	if (!TestNotNull(TEXT("Saved world settings are available"), WorldSettings))
	{
		return false;
	}
	const UClass* MapOverride = WorldSettings->DefaultGameMode.Get();
	TestTrue(TEXT("Saved map override selects the same GameMode Blueprint"), MapOverride == GameModeClass);
	const UClass* EffectiveGameModeClass = MapOverride ? MapOverride : GameModeClass;
	const AGameModeBase* EffectiveGameMode = EffectiveGameModeClass->GetDefaultObject<AGameModeBase>();
	TestTrue(TEXT("Effective map GameMode uses the Blueprint-configured pawn"),
		EffectiveGameMode->DefaultPawnClass == GameMode->DefaultPawnClass);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWizardSceneStructureTest,
	"Wizard.Environment.SceneStructure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FWizardSceneStructureTest::RunTest(const FString& Parameters)
{
	// 读取保存的地图，检查后续系统依赖的入口；不创建或重新布置场景。
	const UWorld* StartupWorld = LoadObject<UWorld>(nullptr, *UGameMapsSettings::GetGameDefaultMap());
	if (!TestNotNull(TEXT("Saved startup world loads"), StartupWorld)
		|| !TestNotNull(TEXT("Startup world has a persistent level"), StartupWorld->PersistentLevel.Get()))
	{
		return false;
	}

	int32 PlayerStarts = 0;
	int32 NavigationBounds = 0;
	int32 EnemySpawnMarkers = 0;
	bool bHasCampShop = false;
	bool bHasChallenge = false;
	bool bHasCampReturn = false;
	bool bHasWalkableGround = false;
	for (const AActor* Actor : StartupWorld->PersistentLevel->Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}
		PlayerStarts += Actor->IsA<APlayerStart>() ? 1 : 0;
		NavigationBounds += Actor->IsA<ANavMeshBoundsVolume>() ? 1 : 0;
		EnemySpawnMarkers += Actor->ActorHasTag(TEXT("Wizard.Arena.Spawn")) ? 1 : 0;
		bHasCampShop |= Actor->ActorHasTag(TEXT("Wizard.Camp.Shop"));
		bHasChallenge |= Actor->ActorHasTag(TEXT("Wizard.Camp.Challenge"));
		bHasCampReturn |= Actor->ActorHasTag(TEXT("Wizard.Camp.Return"));
		if (Actor->GetActorLabel() == TEXT("Ground_Walkable"))
		{
			const AStaticMeshActor* Ground = Cast<AStaticMeshActor>(Actor);
			const UStaticMeshComponent* Component = Ground ? Ground->GetStaticMeshComponent() : nullptr;
			bHasWalkableGround = Component && Component->GetStaticMesh()
				&& Component->GetCollisionEnabled() != ECollisionEnabled::NoCollision;
		}
	}
	TestTrue(TEXT("At least one player spawn is saved"), PlayerStarts > 0);
	TestTrue(TEXT("Navigation bounds are saved"), NavigationBounds > 0);
	TestEqual(TEXT("Three enemy spawn markers remain available"), EnemySpawnMarkers, 3);
	TestTrue(TEXT("Camp shop marker is saved"), bHasCampShop);
	TestTrue(TEXT("Challenge marker is saved"), bHasChallenge);
	TestTrue(TEXT("Camp return marker is saved"), bHasCampReturn);
	TestTrue(TEXT("Walkable ground has a mesh and collision"), bHasWalkableGround);
	return true;
}

#endif
