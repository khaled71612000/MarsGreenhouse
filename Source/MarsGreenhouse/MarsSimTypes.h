// MarsSimTypes.h — shared enums & structs for the Mars Greenhouse sim.
// UE5.3+ . Module: MarsGreenhouse.
#pragma once

#include "CoreMinimal.h"
#include "MarsSimTypes.generated.h"

UENUM(BlueprintType)
enum class EGameState : uint8
{
	Planning   UMETA(DisplayName="Planning"),    // paused, player deciding
	Running    UMETA(DisplayName="Running"),      // sim advancing the sol
	Event      UMETA(DisplayName="Event / Decision"), // paused, awaiting a choice
	Won        UMETA(DisplayName="Won"),
	Lost       UMETA(DisplayName="Lost")
};

UENUM(BlueprintType)
enum class ECropType : uint8
{
	Potato   UMETA(DisplayName="Potato"),
	Lettuce  UMETA(DisplayName="Lettuce"),
	Algae    UMETA(DisplayName="Algae"),
	Legume   UMETA(DisplayName="Peas / Legumes")
};

UENUM(BlueprintType)
enum class EWaterSource : uint8 { IceMining UMETA(DisplayName="Ice mining (ISRU)"), Recycling };

UENUM(BlueprintType)
enum class ELightSource : uint8 { LED, Sun, Hybrid };

UENUM(BlueprintType)
enum class ENutrientSource : uint8 { Compost, Bacteria, EarthOrder UMETA(DisplayName="Order from Earth") };

UENUM(BlueprintType)
enum class EHeatMode : uint8 { Full, Eco };

// Resource identity for fail-reasons / lessons.
UENUM(BlueprintType)
enum class EResource : uint8 { Oxygen, Water, Food, Nitrogen, Power };

// Static, data-driven stats for one crop. Edited in Project Settings (see UMarsSimSettings).
USTRUCT(BlueprintType)
struct FCropStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECropType Type = ECropType::Potato;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float FoodYield   = 60.f;  // % food added at harvest
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float O2Factor    = 1.0f;  // O2 produced per growth unit
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float WaterUse    = 1.0f;  // water consumed per growth unit
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float NitrogenUse = 1.0f;  // negative = fixes nitrogen (legumes)
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float GrowSols    = 6.0f;  // sols from plant to harvest
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float LightNeed   = 1.0f;  // 0..1 light needed for full rate
};

// Runtime state for one greenhouse bed.
USTRUCT(BlueprintType)
struct FPlantedBed
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) bool      bOccupied  = false;
	UPROPERTY(BlueprintReadOnly) bool      bSoilClean = false; // perchlorate washed?
	UPROPERTY(BlueprintReadOnly) ECropType Crop       = ECropType::Potato;
	UPROPERTY(BlueprintReadOnly) float     Growth     = 0.f;   // 0..1
	UPROPERTY(BlueprintReadOnly) float     Health     = 1.f;   // 0..1
};

// ---------------- Event / decision system (Frostpunk-style callouts) ----------------

// What a choice does to the meters (any field can be + or -).
USTRUCT(BlueprintType)
struct FEventEffect
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Oxygen   = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Water    = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Food     = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Nitrogen = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Power    = 0.f;
};

// One of the two options on a card.
USTRUCT(BlueprintType)
struct FEventChoice
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Label;    // the button text
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FEventEffect Effect;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Result;   // optional flavor after choosing
};

// A full astronaut callout with two choices.
USTRUCT(BlueprintType)
struct FEventCard
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Speaker;    // e.g. "Cmdr. Okafor"
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Situation;  // the callout
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FEventChoice ChoiceA;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FEventChoice ChoiceB;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 EarliestSol = 2; // won't fire before this sol
};

// Qualitative arrow summary of an event effect, e.g. "Pwr ^^  O2 v".
// ASCII ^ (up) and v (down) so it always renders in the built-in HUD font;
// use real up/down arrows if you build a UMG event popup.
FORCEINLINE FString EffectArrows(const FEventEffect& E)
{
	auto Arr = [](float v) -> FString
	{
		if (v == 0.f)  return FString();
		if (v >= 8.f)  return TEXT("^^");
		if (v > 0.f)   return TEXT("^");
		if (v <= -8.f) return TEXT("vv");
		return TEXT("v");
	};
	FString R;
	auto Add = [&](const TCHAR* N, float v) { if (v != 0.f) R += FString::Printf(TEXT("%s%s   "), N, *Arr(v)); };
	Add(TEXT("O2"),   E.Oxygen);
	Add(TEXT("H2O"),  E.Water);
	Add(TEXT("Food"), E.Food);
	Add(TEXT("N"),    E.Nitrogen);
	Add(TEXT("Pwr"),  E.Power);
	return R.TrimEnd();
}
