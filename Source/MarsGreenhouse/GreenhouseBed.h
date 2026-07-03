// GreenhouseBed.h — a placeable bed that REFLECTS the sim (shows/hides its plant,
// swaps stage meshes, scales with growth) AND is CLICKABLE in the world like the
// Valorant Sage security-room: click the bed to select it, then use the HUD to act.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GreenhouseBed.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UPrimitiveComponent;
class UGreenhouseSimSubsystem;

UCLASS()
class MARSGREENHOUSE_API AGreenhouseBed : public AActor
{
	GENERATED_BODY()

public:
	AGreenhouseBed();

	/** Which sim bed this represents (0..3). */
	UPROPERTY(EditAnywhere, Category="Bed")
	int32 BedIndex = 0;

	/** Assign meshes for the growth stages, in order (e.g. sprout -> mature). Leave empty to just scale one mesh. */
	UPROPERTY(EditAnywhere, Category="Bed")
	TArray<TObjectPtr<UStaticMesh>> StageMeshes;

	/** Base scale of the plant at full growth. */
	UPROPERTY(EditAnywhere, Category="Bed")
	float BaseScale = 1.f;

	/** Half-size of the invisible click volume around the bed (world units). */
	UPROPERTY(EditAnywhere, Category="Bed")
	FVector ClickExtent = FVector(60.f, 60.f, 40.f);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PlantMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> ClickBox; // receives world clicks

	UFUNCTION() void HandleClicked(AActor* TouchedActor, FKey ButtonPressed);
	UFUNCTION() void HandleCursorOver(UPrimitiveComponent* Comp);
	UFUNCTION() void HandleCursorLeave(UPrimitiveComponent* Comp);

private:
	UGreenhouseSimSubsystem* GetSim() const;
	int32 LastStage = -1;
	bool  bHovered  = false;
};
