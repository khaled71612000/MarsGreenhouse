// GreenhouseViewPawn.h — a fixed camera pawn: no WASD movement, camera switching only.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DefaultPawn.h"
#include "GreenhouseViewPawn.generated.h"

UCLASS()
class MARSGREENHOUSE_API AGreenhouseViewPawn : public ADefaultPawn
{
	GENERATED_BODY()
public:
	AGreenhouseViewPawn();
};
