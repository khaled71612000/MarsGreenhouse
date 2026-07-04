// GreenhouseCamera.h — a static security camera you place and angle. The controller
// collects all of them, cuts between them (fade + sound), and lets you nudge the view
// a few degrees with the mouse. Each one also renders a LIVE PREVIEW (scene capture)
// that the HUD shows as a clickable thumbnail (FNAF-style camera panel).
#pragma once

#include "CoreMinimal.h"
#include "Camera/CameraActor.h"
#include "GreenhouseCamera.generated.h"

class USceneCaptureComponent2D;
class UTextureRenderTarget2D;

UCLASS()
class MARSGREENHOUSE_API AGreenhouseCamera : public ACameraActor
{
	GENERATED_BODY()

public:
	AGreenhouseCamera();

	/** Shown on the HUD, e.g. "Beds", "Airlock", "ISRU". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera")
	FString CameraLabel = TEXT("CAM");

	/** Room/category grouping. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera")
	FString RoomLabel = TEXT("Greenhouse");

	/** How far the mouse can nudge the view, in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera")
	float PanYawLimit = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera")
	float PanPitchLimit = 6.f;

	/** Field of view in degrees. Wider = you see more of the room. 90 is default; 110-120 is roomy. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Camera")
	float CameraFov = 115.f;

	/** Live preview for the HUD thumbnail. Turn off to save performance. */
	UPROPERTY(EditAnywhere, Category="Camera|Preview")
	bool bEnablePreview = true;

	UPROPERTY(EditAnywhere, Category="Camera|Preview")
	int32 PreviewWidth = 320;

	UPROPERTY(EditAnywhere, Category="Camera|Preview")
	int32 PreviewHeight = 180;

	UTextu