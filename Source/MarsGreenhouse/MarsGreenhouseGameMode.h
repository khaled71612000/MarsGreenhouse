// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MarsGreenhouseGameMode.generated.h"

/**
 *  Mars Greenhouse game mode.
 *  Concrete (not abstract) and self-wiring: it sets the PlayerController, a simple
 *  fly-cam pawn, and the on-screen HUD in C++, so no Blueprint GameMode is needed.
 *  Point GlobalDefaultGameMode (or the level's World Settings) at this class.
 */
UCLASS()
class AMarsGreenhouseGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMarsGreenhouseGameMode();
};
