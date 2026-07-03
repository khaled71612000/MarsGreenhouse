// GreenhouseLED.h — the single greenhouse grow-light, made physical. Artists place it over
// the planters; it glows blue / white / red to match the sim's global LedColor, and clicking
// it cycles the light (blue -> balanced -> red). Mirrors the camera system: a placeable,
// clickable world object that reflects and controls sim state.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GreenhouseLED.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UPrimitiveComponent;
class UGreenhouseSimSubsystem;

UCLASS()
class MARSGREENHOUSE_API AGreenhouseLED : public AActor
{
	GENERATED_BODY()

public:
	AGreenhouseLED();

	/** Brightness of the cast light. */
	UPROPERTY(EditAnywhere, Category="LED")
	float Intensity = 6000.f;

	/** Half-size of the invisible click volume. */
	UPROPERTY(EditAnywhere, Category="LED")
	FVector ClickExtent = FVector(80.f, 80.f, 30.f);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Fixture; // clickable body
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Light;

	UFUNCTION() void HandleClicked(AActor* TouchedActor, FKey ButtonPressed);
	UFUNCTION() void HandleCursorOver(UPrimitiveComponent* Comp);
	UFUNCTION() void HandleCursorLeave(UPrimitiveComponent* Comp);

private:
	UGreenhouseSimSubsystem* GetSim() const;
	bool bHovered = false;
};
