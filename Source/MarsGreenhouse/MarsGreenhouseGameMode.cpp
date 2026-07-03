// Copyright Epic Games, Inc. All Rights Reserved.

#include "MarsGreenhouseGameMode.h"
#include "MarsGreenhousePlayerController.h"
#include "MarsGreenhouseHUD.h"
#include "GreenhouseViewPawn.h"

AMarsGreenhouseGameMode::AMarsGreenhouseGameMode()
{
	PlayerControllerClass = AMarsGreenhousePlayerController::StaticClass();
	HUDClass              = AMarsGreenhouseHUD::StaticClass();
	DefaultPawnClass      = AGreenhouseViewPawn::StaticClass(); // static: no movement, camera-switching only
}
