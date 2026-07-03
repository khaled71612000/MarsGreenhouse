// MarsSimTypes.h — shared enums & structs for the Mars Greenhouse sim.
// Turn-based narrative survival. 4 resources, 2 crops with real per-crop life-cycles,
// a manual-harvest ripeness window, and spoilage (bolting / rot).
#pragma once

#include "CoreMinimal.h"
#include "MarsSimTypes.generated.h"

UENUM(BlueprintType)
enum class EGameState : uint8
{
	Planning UMETA(DisplayName="Planning"),
	Event    UMETA(DisplayName="Event / Decision"),
	Won      UMETA(DisplayName="Won"),
	Lost     UMETA(DisplayName="Lost")
};

UENUM(BlueprintType)
enum class ECropType : uint8
{
	Potato  UMETA(DisplayName="Potato"),
	Lettuce UMETA(DisplayName="Lettuce")
};

UENUM(BlueprintType)
enum class ELedColor : uint8
{
	Blue     UMETA(DisplayName="Blue (more O2, slow)"),
	Balanced UMETA(DisplayName="Balanced"),
	Red      UMETA(DisplayName="Red (fast growth, less O2)")
};

UENUM(BlueprintType)
enum class EResource : uint8 { Oxygen, Water, Food, Power };

// Static, data-driven stats for one crop.
USTRUCT(BlueprintType)
struct FCropStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECropType Type = ECropType::Potato;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float FoodYield = 60.f;  // food at a full, healthy harvest
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float O2Factor  = 1.0f;  // O2 produced per growth unit
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float WaterUse  = 1.0f;  // water consumed per growth unit
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float GrowSols  = 8.0f;  // turns from plant to maturity
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ELedColor Prefers = ELedColor::Balanced;

	// Life-cycle: growth fraction (0..1) ending each named stage, and the stage labels.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<float> StageEnds;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FText> StageNames;

	// Harvest window: sols the crop can sit MATURE before it spoils; and the spoil label.
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float RipeWindowSols = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText SpoilName; // e.g. "Bolting" / "Rotting"
};

// Runtime state for one greenhouse planter.
USTRUCT(BlueprintType)
struct FPlantedBed
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) bool      bOccupied = false;
	UPROPERTY(BlueprintReadOnly) ECropType Crop      = ECropType::Potato;
	UPROPERTY(BlueprintReadOnly) float     Growth    = 0.f;   // 0..1 (1 = mature/ready)
	UPROPERTY(BlueprintReadOnly) float     Health    = 1.f;   // 0..1 (keep up by Watering; 0 = dies)
	UPROPERTY(BlueprintReadOnly) float     Overripe  = 0.f;   // sols spent mature-but-unharvested
	UPROPERTY(BlueprintReadOnly) bool      bSpoiled  = false; // bolted / rotted; must be cleared, 0 yield
};

// Which display stage a crop is in for a given growth (index into StageNames/StageMeshes).
FORCEINLINE int32 CropStageIndex(const TArray<float>& Ends, float Growth)
{
	for (int32 i = 0; i < Ends.Num(); ++i)
		if (Growth < Ends[i]) return i;
	return FMath::Max(0, Ends.Num() - 1);
}

// ---------------- Event / narrative system ----------------

USTRUCT(BlueprintType)
struct FEventEffect
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Oxygen = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Water  = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Food   = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float Power  = 0.f;
};

USTRUCT(BlueprintType)
struct FEventChoice
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Label;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FEventEffect Effect;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Result;
};

USTRUCT(BlueprintType)
struct FEventCard
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Speaker;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Situation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FEventChoice ChoiceA;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FEventChoice ChoiceB;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 EarliestSol = 2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 ScriptedSol = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) bool  bSingleChoice = false;
};

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
	auto Add = [&](const TCHAR* N, float v){ if (v != 0.f) R += FString::Printf(TEXT("%s%s   "), N, *Arr(v)); };
	Add(TEXT("O2"),   E.Oxygen);
	Add(TEXT("H2O"),  E.Water);
	Add(TEXT("Food"), E.Food);
	Add(TEXT("Pwr"),  E.Power);
	return R.TrimEnd();
}
