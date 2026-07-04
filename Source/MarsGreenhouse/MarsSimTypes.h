// MarsSimTypes.h — shared enums & structs. Turn-based Arabic-flavored Mars survival ("Bustan").
// 4 resources, 2 crops, one grow-light (Purple / White), manual harvest + spoilage, crew flavor.
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

// Grow-light. Purple = CEA "blurple" red+blue mix (more oxygen, steady growth).
// White = full spectrum (faster growth, a little less oxygen).
UENUM(BlueprintType)
enum class ELedColor : uint8
{
	Purple UMETA(DisplayName="Purple (efficient, more O2)"),
	White  UMETA(DisplayName="White (fast growth, less O2)")
};

UENUM(BlueprintType)
enum class EResource : uint8 { Oxygen, Water, Food, Power };

USTRUCT(BlueprintType)
struct FCropStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite) ECropType Type = ECropType::Potato;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float FoodYield = 60.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float O2Factor  = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float WaterUse  = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float GrowSols  = 8.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) ELedColor Prefers = ELedColor::Purple;

	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<float> StageEnds;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FText> StageNames;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) float RipeWindowSols = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText SpoilName;
};

USTRUCT(BlueprintType)
struct FPlantedBed
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) bool      bOccupied = false;
	UPROPERTY(BlueprintReadOnly) ECropType Crop      = ECropType::Potato;
	UPROPERTY(BlueprintReadOnly) float     Growth    = 0.f;
	UPROPERTY(BlueprintReadOnly) float     Health    = 1.f;
	UPROPERTY(BlueprintReadOnly) float     Overripe  = 0.f;
	UPROPERTY(BlueprintReadOnly) bool      bSpoiled  = false;
};

// One crew member (flavor roster - no gameplay effect).
USTRUCT(BlueprintType)
struct FCrewMember
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Name;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Role;
};

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
