// Copyright Epic Games, Inc. All Rights Reserved.

#include "WizardGameMode.h"
#include "WizardPlayerController.h"
#include "WizardPlayerState.h"
#include "Save/WizardSaveComponent.h"
#include "Challenge/WizardChallengeArena.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

AWizardGameMode::AWizardGameMode()
{
	// 业务类型提供原生默认值；角色资源由 GameMode 子蓝图配置。
	PlayerControllerClass = AWizardPlayerController::StaticClass();
	PlayerStateClass = AWizardPlayerState::StaticClass();
}

void AWizardGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);
	AWizardPlayerState* Player = NewPlayer ? NewPlayer->GetPlayerState<AWizardPlayerState>() : nullptr;
	if (!Player || !NewPlayer->GetPawn() || Player->GetSaveService()->GetStatus() != EWizardSaveStatus::NotStarted) return;
	// 编辑器“从此处开始”也回到已配置营地；不改地图或持久化角色位置。
	for (TActorIterator<AWizardChallengeArena> It(GetWorld()); It; ++It)
	{
		if (ACharacter* Avatar = Cast<ACharacter>(NewPlayer->GetPawn()))
		{
			Avatar->GetCharacterMovement()->StopMovementImmediately();
			if (!Avatar->TeleportTo(It->CampReturnLocation, It->CampReturnRotation, false, false))
			{
				UE_LOG(LogTemp, Warning, TEXT("读档营地返回点被阻挡，沿用挑战返回位置。"));
				Avatar->SetActorLocationAndRotation(It->CampReturnLocation, It->CampReturnRotation, false, nullptr, ETeleportType::TeleportPhysics);
			}
		}
		break;
	}
	Player->GetSaveService()->StartSession();
}
