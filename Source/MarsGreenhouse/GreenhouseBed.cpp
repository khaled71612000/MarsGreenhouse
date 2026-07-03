// GreenhouseBed.cpp
#include "GreenhouseBed.h"
#include "GreenhouseSimSubsystem.h"
#include "MarsGreenhousePlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AGreenhouseBed::AGreenhouseBed()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	PlantMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlantMesh"));
	PlantMesh->SetupAttachment(Root);
	PlantMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Invisible volume that catches mouse clicks (the Sage-room interaction).
	ClickBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ClickBox"));
	ClickBox->SetupAttachment(Root);
	ClickBox->SetBoxExtent(ClickExtent);
	ClickBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ClickBox->SetCollisionObjectType(ECC_WorldDynamic);
	ClickBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	ClickBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block); // default click trace channel
	ClickBox->SetHiddenInGame(true);

	// placeholder cube so it shows immediately; assign StageMeshes to override.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) PlantMesh->SetStaticMesh(Cube.Object);
}

void AGreenhouseBed::BeginPlay()
{
	Super::BeginPlay();
	if (ClickBox) ClickBox->SetBoxExtent(ClickExtent);
	OnClicked.AddDynamic(this, &AGreenhouseBed::HandleClicked);
	if (ClickBox)
	{
		ClickBox->OnBeginCursorOver.AddDynamic(this, &AGreenhouseBed::HandleCursorOver);
		ClickBox->OnEndCursorOver.AddDynamic(this, &AGreenhouseBed::HandleCursorLeave);
	}
}

void AGreenhouseBed::HandleClicked(AActor* /*TouchedActor*/, FKey /*ButtonPressed*/)
{
	if (auto* PC = Cast<AMarsGreenhousePlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
		PC->SelectBedFromWorld(BedIndex);
}

void AGreenhouseBed::HandleCursorOver(UPrimitiveComponent*) { bHovered = true; }
void AGreenhouseBed::HandleCursorLeave(UPrimitiveComponent*) { bHovered = false; }

UGreenhouseSimSubsystem* AGreenhouseBed::GetSim() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UGreenhouseSimSubsystem>() : nullptr;
}

void AGreenhouseBed::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const UGreenhouseSimSubsystem* S = GetSim();
	if (!S || !S->Beds.IsValidIndex(BedIndex)) { PlantMesh->SetVisibility(false); return; }

	const FPlantedBed& B = S->Beds[BedIndex];
	if (!B.bOccupied) { PlantMesh->SetVisibility(false); LastStage = -1; return; }

	PlantMesh->SetVisibility(true);

	// Growth is shown by SWAPPING the stage mesh, not by scaling. Assign 8 meshes in order
	// (Seed Tuber -> Sprouting -> Root Dev -> Vegetative -> Tuber Initiation -> Bulking ->
	//  Maturation -> Harvest) and the correct one shows for the plant's current growth.
	const int32 N = StageMeshes.Num();
	if (N > 0)
	{
		const int32 Stage = FMath::Clamp((int32)(B.Growth * N), 0, N - 1);
		if (Stage != LastStage && StageMeshes[Stage])
		{
			PlantMesh->SetStaticMesh(StageMeshes[Stage]);
			LastStage = Stage;
		}
	}

	// Fixed size — the mesh changes, not the scale.
	PlantMesh->SetRelativeScale3D(FVector(BaseScale));
}
