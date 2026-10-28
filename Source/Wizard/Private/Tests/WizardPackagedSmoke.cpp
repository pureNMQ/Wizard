// 开发包的启动与移动检查；只通过显式命令执行，Shipping/Test 构建不注册此命令。
#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST

#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "UObject/Package.h"
#include "WizardCharacter.h"
#include "WizardGameMode.h"
#include "WizardPlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogWizardSmoke, Log, All);

namespace WizardSmoke
{
	void Fail(const TCHAR* Reason)
	{
		UE_LOG(LogWizardSmoke, Error, TEXT("WIZARD_SMOKE_FAIL: %s"), Reason);
		FPlatformMisc::RequestExitWithStatus(false, 1);
	}

	void Run(UWorld* World)
	{
		// 防止误在编辑器/PIE 世界中关闭编辑器进程。
		if (!World || World->WorldType != EWorldType::Game)
		{
			UE_LOG(LogWizardSmoke, Warning, TEXT("Smoke command only runs in a standalone game world."));
			return;
		}
		if (World->GetOutermost()->GetName() != TEXT("/Game/Wizard/Maps/L_WizardClearing"))
		{
			Fail(TEXT("Unexpected startup map"));
			return;
		}
		APlayerController* Controller = World->GetFirstPlayerController();
		AWizardCharacter* Character = Controller ? Cast<AWizardCharacter>(Controller->GetPawn()) : nullptr;
		if (!World->GetAuthGameMode<AWizardGameMode>()
			|| !Cast<AWizardPlayerController>(Controller) || !Character)
		{
			Fail(TEXT("GameMode, controller or possessed player pawn is missing"));
			return;
		}

		float WaitSeconds = 10.0f;
		FParse::Value(FCommandLine::Get(), TEXT("WizardSmokeSeconds="), WaitSeconds);
		WaitSeconds = FMath::Clamp(WaitSeconds, 2.0f, 60.0f);
		const FVector Start = Character->GetActorLocation();
		const TWeakObjectPtr<AWizardCharacter> WeakCharacter(Character);
		UE_LOG(LogWizardSmoke, Display, TEXT("WIZARD_SMOKE_BEGIN: GameMode=%s Pawn=%s Start=%s"),
			*World->GetAuthGameMode()->GetClass()->GetPathName(), *Character->GetClass()->GetName(), *Start.ToString());
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(Controller, Start + FVector(300.0f, 0.0f, 0.0f));

		FTimerHandle CompletionHandle;
		World->GetTimerManager().SetTimer(CompletionHandle, FTimerDelegate::CreateLambda([WeakCharacter, Start]()
		{
			const AWizardCharacter* Player = WeakCharacter.Get();
			if (!Player)
			{
				Fail(TEXT("Player pawn was destroyed during startup"));
				return;
			}
			const float Distance = FVector::Dist2D(Start, Player->GetActorLocation());
			if (Distance < 50.0f || !Player->GetCharacterMovement()->IsMovingOnGround())
			{
				Fail(TEXT("Player did not move on the walkable scene"));
				return;
			}
			UE_LOG(LogWizardSmoke, Display, TEXT("WIZARD_SMOKE_PASS: MovedCm=%.2f Grounded=true"), Distance);
			FPlatformMisc::RequestExitWithStatus(false, 0);
		}), WaitSeconds, false);
	}

	FAutoConsoleCommandWithWorld Command(TEXT("Wizard.SmokeTest"),
		TEXT("Verify the packaged startup scene, player and navigation movement, then exit."),
		FConsoleCommandWithWorldDelegate::CreateStatic(&Run));
}

#endif
