// GreenhouseLED.cpp — the greenhouse grow-light. Glows PURPLE or WHITE to match the sim.
#include "GreenhouseLED.h"
#include "GreenhouseSimSubsystem.h"
#include "MarsGreenhousePlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AGreenhouseLED::AGreenhouseLED()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Fixture = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Fixture"));
	Fixture->SetupAttachment(Root);
	Fixture->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Fixture->SetCollisionObjectType(ECC_WorldDynamic);
	Fixture->SetCollisionResponseToAllChannels(ECR_Ignore);
	Fixture->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (Cube.Succeeded()) Fixture->SetStaticMesh(Cube.Object);
	Fixture->SetRelativeScale3D(FVector(1.6f, 1.6f, 0.2f));

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetRelativeLocation(FVector(0.f, 0.f, -40.f));
	Light->SetAttenuationRadius(1200.f);
}

void AGreenhouseLED::BeginPlay()
{
	Super::BeginPlay();
	OnClicked.AddDynamic(this, &AGreenhouseLED::HandleClicked);
	if (Fixture)
	{
		Fixture->OnBeginCursorOver.AddDynamic(this, &AGreenhouseLED::HandleCursorOver);
		Fixture->OnEndCursorOver.AddDynamic(this, &AGreenhouseLED::HandleCursorLeave);
	}
}

void AGreenhouseLED::HandleClicked(AActor*, FKey)
{
	if (auto* PC = Cast<AMarsGreenhousePlayerController>(UGameplayStatics::GetPlayerController(this, 0)))
		PC->HandleHudAction(FName("led_cycle"));
}

void AGreenhouseLED::HandleCursorOver(UPrimitiveComponent*) { bHovered = true; }
void AGreenhouseLED::HandleCursorLeave(UPrimitiveComponent*) { bHovered = false; }

UGreenhouseSimSubsystem* AGreenhouseLED::GetSim() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UGreenhouseSimSubsystem>() : nullptr;
}

void AGreenhouseLED::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	const UGreenhouseSimSubsystem* S = GetSim();
	if (!S || !Light) return;

	// Only the COLOR is driven by the sim. Intensity, attenuation radius, and every other
	// light setting stay exactly as the artist configured them on the placed light in the editor.
	const FLinearColor C = (S->LedColor == ELedColor::White)
		? FLinearColor(1.0f, 0.98f, 0.94f)
		: FLinearColor(0.62f, 0.28f, 0.98f); // purple "blurple"
	Light->SetLightColor(C);
}
