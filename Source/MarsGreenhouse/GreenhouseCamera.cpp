// GreenhouseCamera.cpp
#include "GreenhouseCamera.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Camera/CameraComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"

AGreenhouseCamera::AGreenhouseCamera()
{
	Capture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("Preview Capture"));
	if (GetCameraComponent()) Capture->SetupAttachment(GetCameraComponent());
	Capture->CaptureSource      = SCS_FinalColorLDR;
	Capture->bCaptureEveryFrame = true;
	Capture->bCaptureOnMovement = false;
	Capture->ProjectionType     = ECameraProjectionMode::Perspective;
	Capture->FOVAngle           = 75.f;
}

void AGreenhouseCamera::BeginPlay()
{
	Super::BeginPlay();

	// Widen the view so the player can actually see the room (runs even if preview is off).
	if (GetCameraComponent()) GetCameraComponent()->SetFieldOfView(CameraFov);

	if (!bEnablePreview)
	{
		if (Capture) Capture->bCaptureEveryFrame = false;
		return;
	}

	PreviewRT = NewObject<UTextureRenderTarget2D>(this);
	PreviewRT->RenderTargetFormat = RTF_RGBA8;
	PreviewRT->InitAutoFormat(FMath::Max(64, PreviewWidth), FMath::Max(36, PreviewHeight));
	PreviewRT->UpdateResourceImmediate(true);

	if (Capture)
	{
		Capture->TextureTarget = PreviewRT;
		Capture->FOVAngle = CameraFov;   // preview matches the wide camera
	}
}
