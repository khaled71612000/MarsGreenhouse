// GreenhouseLabel.h — a placeable, non-interactive world label. Drop it on a prop (Water Tank,
// Airlock, Oxygen Generator...) and set the text; the HUD projects it to screen and draws a
// floating tag with a small leader line. Purely cosmetic.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GreenhouseLabel.generated.h"

class UBillboardComponent;

UCLASS()
class MARSGREENHOUSE_API AGreenhouseLabel : public AActor
{
	GENERATED_BODY()

public:
	AGreenhouseLabel();

	/** The tag text shown in the HUD, e.g. "Water Tank". */
	UPROPERTY(EditAnywhere, Category="Label")
	FString LabelText = TEXT("Label");

	/** Screen-height offset (world units) so the tag floats above the prop. */
	UPROPERTY(EditAnywhere, Category="Label")
	float HeightOffset = 90.f;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UBillboardComponent> Icon;
};
