// GreenhouseViewPawn.cpp
#include "GreenhouseViewPawn.h"

AGreenhouseViewPawn::AGreenhouseViewPawn()
{
	// Kill the default WASD/Q/E fly bindings — you switch cameras, you don't move.
	bAddDefaultMovementBindings = false;
	PrimaryActorTick.bCanEverTick = false;
}
