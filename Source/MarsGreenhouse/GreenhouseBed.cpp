// GreenhouseBed.cpp
#include "GreenhouseBed.h"
#include "GreenhouseSimSubsystem.h"
#include "MarsSimSettings.h"
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

	ClickBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ClickBox"));
	ClickBox->SetupAttachment(Root);
	ClickBox->SetBoxExtent(ClickExtent);
	ClickBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ClickBox->SetCollisionObjectType(ECC_WorldDynamic);
	ClickBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	ClickBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	ClickBox->SetHiddenInGame(true);

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

void AGreenhouseBed::HandleClicked(AActor*, FKey)
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
	if (!B.bOccupied) { PlantMesh->SetVisibility(false); LastMesh = nullptr; return; }

	PlantMesh->SetVisibility(true);

	// Pick the mesh: spoiled -> SpoiledMesh; else the per-crop stage mesh.
	UStaticMesh* Target = nullptr;
	if (B.bSpoiled && SpoiledMesh)
	{
		Target = SpoiledMesh;
	}
	else
	{
		const TArray<TObjectPtr<UStaticMesh>>& Set = (B.Crop == ECropType::Potato) ? PotatoStageMeshes : LettuceStageMeshes;
		if (Set.Num() > 0)
		{
			const FCropStats& C = GetDefault<UMarsSimSettings>()->GetCrop(B.Crop);
			const int32 Stage = FMath::Clamp(CropStageIndex(C.StageEnds, B.Growth), 0, Set.Num() - 1);
			Target = Set[Stage];
		}
	}

	if (Target && Target != LastMesh) { PlantMesh->SetStaticMesh(Target); LastMesh = Target; }
	PlantMesh->SetRelativeScale3D(FVector(BaseScale)); // mesh changes, not size
}
