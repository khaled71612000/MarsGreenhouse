// GreenhouseBed.h — a placeable planter that REFLECTS the sim and is CLICKABLE in the world.
// Growth is shown by SWAPPING per-crop stage meshes (not scaling). Potato and lettuce have
// their own mesh sets; a spoiled crop shows the SpoiledMesh (bolted lettuce / rotted potato).
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

	/** Which sim planter this represents (0..3). */
	UPROPERTY(EditAnywhere, Category="Planter")
	int32 BedIndex = 0;

	/** Potato stage meshes in order: Sprouting, Stolons, Tuber bulking, Ready. */
	UPROPERTY(EditAnywhere, Category="Planter")
	TArray<TObjectPtr<UStaticMesh>> PotatoStageMeshes;

	/** Lettuce stage meshes in order: Seed, Germination, Vegetative, Ready. */
	UPROPERTY(EditAnywhere, Category="Planter")
	TArray<TObjectPtr<UStaticMesh>> LettuceStageMeshes;

	/** Optional mesh shown when the crop spoils (bolted / rotted). */
	UPROPERTY(EditAnywhere, Category="Planter")
	TObjectPtr<UStaticMesh> SpoiledMesh;

	/** Fixed display scale (the mesh changes, not the size). */
	UPROPERTY(EditAnywhere, Category="Planter")
	float BaseScale = 1.f;

	/** Half-size of the invisible click volume around the planter. */
	UPROPERTY(EditAnywhere, Category="Planter")
	FVector ClickExtent = FVector(60.f, 60.f, 40.f);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> PlantMesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> ClickBox;

	UFUNCTION() void HandleClicked(AActor* TouchedActor, FKey ButtonPressed);
	UFUNCTION() void HandleCursorOver(UPrimitiveComponent* Comp);
	UFUNCTION() void HandleCursorLeave(UPrimitiveComponent* Comp);

private:
	UGreenhouseSimSubsystem* GetSim() const;
	UStaticMesh* LastMesh = nullptr;
	bool  bHovered = false;
};
