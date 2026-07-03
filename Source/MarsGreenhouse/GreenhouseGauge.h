// GreenhouseGauge.h — a placeable physical gauge that reflects ONE meter. Assign a
// mesh (a cube whose pivot is at its base works best) and pick the resource; it scales
// on Z with the value. Place as many as you want to make the HUD "physical".
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MarsSimTypes.h"
#include "GreenhouseGauge.generated.h"

class UStaticMeshComponent;
class UGreenhouseSimSubsystem;

UCLASS()
class MARSGREENHOUSE_API AGreenhouseGauge : public AActor
{
	GENERATED_BODY()

public:
	AGreenhouseGauge();

	UPROPERTY(EditAnywhere, Category="Gauge")
	EResource Resource = EResource::Oxygen;

	/** Z scale at 100%. */
	UPROPERTY(EditAnywhere, Category="Gauge")
	float FullScaleZ = 2.f;

protected:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> FillMesh;

private:
	UGreenhouseSimSubsystem* GetSim() const;
};
