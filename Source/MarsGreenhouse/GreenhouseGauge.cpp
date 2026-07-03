// GreenhouseGauge.cpp
#include "GreenhouseGauge.h"
#include "GreenhouseSimSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AGreenhouseGauge::AGreenhouseGauge()
{
	// DISABLED: the in-world resource bar was removed per design (the HUD already shows meters).
	// The actor is kept so existing level placements don't break, but it renders nothing and
	// does not tick. Delete the placed GreenhouseGauge actors in the level to remove entirely.
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	FillMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FillMesh"));
	FillMesh->SetupAttachment(Root);
	FillMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FillMesh->SetVisibility(false);
	FillMesh->SetHiddenInGame(true);
}

UGreenhouseSimSubsystem* AGreenhouseGauge::GetSim() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UGreenhouseSimSubsystem>() : nullptr;
}

void AGreenhouseGauge::Tick(float DeltaTime)
{
	// Disabled: the in-world resource bar was removed. Kept as a no-op so old level
	// placements still load. Delete the actors in the level to remove entirely.
	Super::Tick(DeltaTime);
}
