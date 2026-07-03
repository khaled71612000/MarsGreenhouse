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
	Super::Tick(DeltaTime);
	const UGreenhouseSimSubsystem* S = GetSim();
	if (!S) return;

	float V = 0.f;
	switch (Resource)
	{
		case EResource::Oxygen:   V = S->Oxygen;   break;
		case EResource::Water:    V = S->Water;    break;
		case EResource::Food:     V = S->Food;     break;
		case EResource::Nitrogen: V = S->Nitrogen; break;
		default:                  V = S->Power;    break;
	}
	const float T = FMath::Clamp(V * 0.01f, 0.02f, 1.f);
	FillMesh->SetRelativeScale3D(FVector(1.f, 1.f, T * FullScaleZ));
}
