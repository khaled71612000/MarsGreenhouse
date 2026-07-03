// MarsSimSettings.h — all tunable constants, exposed in Project Settings -> Game -> "Mars Greenhouse Sim".
// Designers tune the whole game here with NO recompile (saved to DefaultGame.ini).
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MarsSimTypes.h"
#include "MarsSimSettings.generated.h"

class UUserWidget;
class UEventPopupWidget;
class USoundBase;
class UFont;

UCLASS(config=Game, defaultconfig, meta=(DisplayName="Mars Greenhouse Sim"))
class MARSGREENHOUSE_API UMarsSimSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMarsSimSettings();

	virtual FName GetCategoryName() const override { return FName("Game"); }

	// --- Structure ---
	UPROPERTY(config, EditAnywhere, Category="Time")  int32 TotalSols = 15;
	UPROPERTY(config, EditAnywhere, Category="Time")  int32 NumBeds   = 4;

	// --- Crew — per-sol consumption in meter units ---
	UPROPERTY(config, EditAnywhere, Category="Crew")  int32 CrewCount   = 6;
	UPROPERTY(config, EditAnywhere, Category="Crew")  int32 GraceSols    = 1;    // sols a meter may sit at 0 before fail
	UPROPERTY(config, EditAnywhere, Category="Crew")  float O2PerCrew    = 2.2f;
	UPROPERTY(config, EditAnywhere, Category="Crew")  float FoodPerCrew  = 1.3f;
	UPROPERTY(config, EditAnywhere, Category="Crew")  float WaterPerCrew = 1.1f;

	// --- Starting meters ---
	UPROPERTY(config, EditAnywhere, Category="Start") float StartOxygen = 60.f;
	UPROPERTY(config, EditAnywhere, Category="Start") float StartWater  = 70.f;
	UPROPERTY(config, EditAnywhere, Category="Start") float StartFood   = 55.f;
	UPROPERTY(config, EditAnywhere, Category="Start") float StartPower  = 65.f;

	// --- Power ---
	UPROPERTY(config, EditAnywhere, Category="Power") float SolarBasePerSol = 40.f;
	UPROPERTY(config, EditAnywhere, Category="Power") float LedDrawPerSol   = 9.f; // the grow-light always runs

	// --- Water ---
	UPROPERTY(config, EditAnywhere, Category="Water") float RecycleYieldPerSol = 10.f; // passive closed-loop recovery

	// --- Growth ---
	UPROPERTY(config, EditAnywhere, Category="Growth") float PhotosynthesisRate = 1.0f; // global growth scalar
	UPROPERTY(config, EditAnywhere, Category="Growth") float GrowthMeterScale   = 12.f; // growth-units -> meter units
	UPROPERTY(config, EditAnywhere, Category="Growth") float HealthDecayPerSol  = 0.10f;

	// --- Grow-light (LED) multipliers ---
	UPROPERTY(config, EditAnywhere, Category="Light") float LedGrowthBlue     = 0.90f;
	UPROPERTY(config, EditAnywhere, Category="Light") float LedGrowthBalanced = 1.05f;
	UPROPERTY(config, EditAnywhere, Category="Light") float LedGrowthRed      = 1.25f;
	UPROPERTY(config, EditAnywhere, Category="Light") float LedO2Blue         = 1.30f;
	UPROPERTY(config, EditAnywhere, Category="Light") float LedO2Balanced     = 1.00f;
	UPROPERTY(config, EditAnywhere, Category="Light") float LedO2Red          = 0.70f;
	UPROPERTY(config, EditAnywhere, Category="Light") float LedMatchBonus     = 0.15f; // if light matches crop.Prefers

	// --- Active acquisition actions (each spends power; power is the master limiter) ---
	UPROPERTY(config, EditAnywhere, Category="Actions") float MineIceWater         = 18.f;
	UPROPERTY(config, EditAnywhere, Category="Actions") float MineIcePowerCost     = 12.f;
	UPROPERTY(config, EditAnywhere, Category="Actions") float ElectrolyzeOxygen    = 14.f;
	UPROPERTY(config, EditAnywhere, Category="Actions") float ElectrolyzeWaterCost = 10.f;
	UPROPERTY(config, EditAnywhere, Category="Actions") float ElectrolyzePowerCost = 8.f;
	UPROPERTY(config, EditAnywhere, Category="Actions") float WaterPlantWaterCost  = 6.f;  // "Water" plant care
	UPROPERTY(config, EditAnywhere, Category="Actions") float WaterPlantPowerCost  = 2.f;
	UPROPERTY(config, EditAnywhere, Category="Actions") float WaterPlantHealth     = 0.35f;

	// --- Dust event (lowers solar) ---
	UPROPERTY(config, EditAnywhere, Category="Dust") float DustChancePerSol = 0.18f;
	UPROPERTY(config, EditAnywhere, Category="Dust") float DustMagnitudeMin = 0.40f;
	UPROPERTY(config, EditAnywhere, Category="Dust") float DustMagnitudeMax = 0.70f;

	// --- Difficulty ramp (harder + bigger rewards as the run goes on) ---
	UPROPERTY(config, EditAnywhere, Category="Difficulty") int32 DiffStep1Sol = 5;
	UPROPERTY(config, EditAnywhere, Category="Difficulty") int32 DiffStep2Sol = 10;
	UPROPERTY(config, EditAnywhere, Category="Difficulty") float DiffStep1Mult = 1.25f; // consumption + dust scale
	UPROPERTY(config, EditAnywhere, Category="Difficulty") float DiffStep2Mult = 1.50f;
	UPROPERTY(config, EditAnywhere, Category="Difficulty") float RewardRampPerStep = 0.20f; // crop yield bonus per step

	// --- Crops (2) ---
	UPROPERTY(config, EditAnywhere, Category="Crops") TArray<FCropStats> Crops;

	// --- Events (scripted story beats + random pool) ---
	UPROPERTY(config, EditAnywhere, Category="Events") float EventChancePerSol = 0.4f; // for the random pool
	UPROPERTY(config, EditAnywhere, Category="Events") TArray<FEventCard> Events;

	// --- Day-start fun facts (accurate Mars / CEA science) ---
	UPROPERTY(config, EditAnywhere, Category="Narrative") TArray<FText> FunFacts;

	// --- Optional custom HUD font. Assign a Font asset (import a .ttf) for a modern look; empty = engine font. ---
	UPROPERTY(config, EditAnywhere, Category="UI") TSoftObjectPtr<UFont> HudFont;

	// --- Optional custom UI. Leave empty to use the built-in C++ HUD. ---
	UPROPERTY(config, EditAnywhere, Category="UI") TSoftClassPtr<UUserWidget> DashboardWidgetClass;
	UPROPERTY(config, EditAnywhere, Category="UI") TSoftClassPtr<UEventPopupWidget> EventPopupWidgetClass;

	// --- Optional audio. Assign your own; empty = silent. ---
	UPROPERTY(config, EditAnywhere, Category="Audio") TSoftObjectPtr<USoundBase> MusicLoop;
	UPROPERTY(config, EditAnywhere, Category="Audio") TSoftObjectPtr<USoundBase> SfxClick;
	UPROPERTY(config, EditAnywhere, Category="Audio") TSoftObjectPtr<USoundBase> SfxConfirm;
	UPROPERTY(config, EditAnywhere, Category="Audio") TSoftObjectPtr<USoundBase> SfxAlert;
	UPROPERTY(config, EditAnywhere, Category="Audio") TSoftObjectPtr<USoundBase> SfxTransition;

	const FCropStats& GetCrop(ECropType Type) const;
};
