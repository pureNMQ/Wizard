// Copyright Epic Games, Inc. All Rights Reserved.

#include "WizardGameMode.h"
#include "WizardPlayerController.h"
#include "WizardPlayerState.h"

AWizardGameMode::AWizardGameMode()
{
	// 业务类型提供原生默认值；角色资源由 GameMode 子蓝图配置。
	PlayerControllerClass = AWizardPlayerController::StaticClass();
	PlayerStateClass = AWizardPlayerState::StaticClass();
}
