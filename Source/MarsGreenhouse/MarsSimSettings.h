// MarsSimSettings.h — all tunable constants, exposed in Project Settings -> Game -> "Mars Greenhouse Sim".
// Designers tune the whole simulation here with NO recompile (saved to DefaultGame.ini).
// game-developer rule: data-driven values, never hardcoded magic numbers in logic.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MarsSimTypes.h"
#include "MarsSimSettings.generated.h"

class UUserWidget;
class UEventPopupWidget;
class USoundBase;

UCLASS(config=Game, defaultconfig, meta=(DisplayName="Mars Greenhouse Sim"))
class MARSGREENHOUSE_API UMarsSimSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMarsSimSettings();

	virtual FName GetCategoryName() const override { return FName("Game"); }

	// --- Time / structure ---
	UPROPERTY(config, EditAnywhere, Category="Time")  float SolLengthSeconds = 60.f;  // real seconds per sol when running
	UPROPERTY(config, EditAnywhere, Category="Time")  int32 TotalSols        = 15;
	UPROPERTY(config, EditAnywhere, Category="Time")  int32 NumBeds          = 4;

	// --- Crew (8 astronauts) — per-sol consumption in meter % ---
	UPROPERTY(config, EditAnywhere, Category="Crew")  int32 CrewCount   = 8;
	UPROPERTY(config, EditAnywhere, Category="Crew")  int32 GraceSols    = 1;     // sols a meter may sit at 0 before fail
	UPROPERTY(config, EditAnywhere, Category="Crew")  float O2PerCrew    = 2.25f;
	UPROPERTY(config, EditAnywhere, Category="Crew")  float FoodPerCrew  = 1.25f;
	UPROPERTY(config, EditAnywhere, Category="Crew")  float WaterPerCrew = 1.00f;

	// --- Starting meters (Nitrogen starts low = the "leak" scenario) ---
	UPROPERTY(config, EditAnywhere, Category="Start") float StartOxygen   = 60.f;
	UPROPERTY(config, EditAnywhere, Category="Start") float StartWater    = 70.f;
	UPROPERTY(config, EditAnywhere, Category="Start") float StartFood     = 55.f;
	UPROPERTY(config, EditAnywhere, Category="Start") float StartNitrogen = 40.f;
	UPROPERTY(config, EditAnywhere, Category="Start") float StartPower    = 65.f;

	// --- Power ---
	UPROPERTY(config, EditAnywhere, Category="Power") float SolarBasePerSol = 42.f;
	UPROPERTY(config, EditAnywhere, Category="Power") float LedDrawPerSol   = 10.f;
	UPROPERTY(config, EditAnywhere, Category="Power") float HeaterFullPerSol= 8.f;
	UPROPERTY(config, EditAnywhere, Category="Power") float HeaterEcoPerSol = 4.f;
	UPROPERTY(config, EditAnywhere, Category="Power") float IsruDrawPerSol  = 15.f;
	UPROPERTY(config, EditAnywhere, Category="Power") float RecycleDrawPerSol = 3.f;
	UPROPERTY(config, EditAnywhere, Category="Power") float BacteriaDrawPerSol = 6.f;

	// --- Water ---
	UPROPERTY(config, EditAnywhere, Category="Water") float IceYieldPerSol     = 30.f;
	UPROPERTY(config, EditAnywhere, Category="Water") float RecycleYieldPerSol = 12.f;
	UPROPERTY(config, EditAnywhere, Category="Water") float SoilWashWaterCost  = 15.f;

	// --- Light / photosynthesis ---
	UPROPERTY(config, EditAnywhere, Category="Light") float SunlightFraction  = 0.43f; // Mars vs Earth
	UPROPERTY(config, EditAnywhere, Category="Light") float HybridLight       = 0.70f;
	UPROPERTY(config, EditAnywhere, Category="Growth") float PhotosynthesisRate = 1.0f; // global growth scalar
	UPROPERTY(config, EditAnywhere, Category="Growth") float EcoGrowthPenalty   = 0.70f; // growth mult in Eco heat

	// --- Nutrients (nitrogen per sol) ---
	UPROPERTY(config, EditAnywhere, Category="Nutrients") float CompostNitrogenPerSol  = 2.5f;
	UPROPERTY(config, EditAnywhere, Category="Nutrients") float BacteriaNitrogenPerSol = 6.0f;
	UPROPERTY(config, EditAnywhere, Category="Nutrients") float EarthOrderNitrogen     = 25.f; // per use
	UPROPERTY(config, EditAnywhere, Category="Nutrients") int32 EarthOrderCredits      = 2;    // limited supply drops

	// --- Active acquisition actions (player-triggered; each spends power) ---
	// These let the player RECOVER a resource, not just slow its decline. Power is the master resource.
	UPROPERTY(config, EditAnywhere, Category="Actions") int32 MaxActionsPerSol     = 3;    // limits spamming actions each sol
	UPROPERTY(config, EditAnywhere, Category="Actions") float MineIceWater         = 18.f; // water gained per use
	UPROPERTY(config, EditAnywhere, Category="Actions") float MineIcePowerCost     = 12.f;
	UPROPERTY(config, EditAnywhere, Category="Actions") float ElectrolyzeOxygen    = 14.f; // O2 gained per use
	UPROPERTY(config, EditAnywhere, Category="Actions") float ElectrolyzeWaterCost = 10.f; // splits water...
	UPROPERTY(config, EditAnywhere, Category="Actions") float ElectrolyzePowerCost = 8.f;  // ...costs power too
	UPROPERTY(config, EditAnywhere, Category="Actions") float HarvestN2Nitrogen    = 12.f; // N gained per use
	UPROPERTY(config, EditAnywhere, Category="Actions") float HarvestN2PowerCost   = 10.f;

	// --- Dust event (lowers solar + light) ---
	UPROPERTY(config, EditAnywhere, Category="Events") float DustChancePerSol = 0.18f;
	UPROPERTY(config, EditAnywhere, Category="Events") float DustMagnitudeMin = 0.40f;
	UPROPERTY(config, EditAnywhere, Category="Events") float DustMagnitudeMax = 0.70f;

	// --- Crops (defaults set in constructor; tune per-crop here) ---
	UPROPERTY(config, EditAnywhere, Category="Crops") TArray<FCropStats> Crops;

	// --- Events (astronaut callouts / dramatic choices) ---
	UPROPERTY(config, EditAnywhere, Category="Events") float EventChancePerSol = 0.5f; // 0..1
	UPROPERTY(config, EditAnywhere, Category="Events") TArray<FEventCard> Events;

	// --- Optional custom UI. Leave empty to use the built-in C++ HUD (per-piece). ---
	UPROPERTY(config, EditAnywhere, Category="UI") TSoftClassPtr<UUserWidget> DashboardWidgetClass;
	UPROPERTY(config, EditAnywhere, Category="UI") TSoftClassPtr<UEventPopupWidget> EventPopupWidgetClass;

	// --- Optional audio. Assign your own; empty = silent. ---
	UPROPERTY(config, EditAnywhere, Category="Audio") TSoftObjectPtr<USoundBase> MusicLoop;      // looping bg music asset
	UPROPERTY(config, EditAnywhere, Category="Audio") TSoftObjectPtr<USoundBase> SfxClick;       // generic action
	UPROPERTY(config, EditAnywhere, Category="Audio") TSoftObjectPtr<USoundBase> SfxConfirm;     // plant / wash / harvest
	UPROPERTY(config, EditAnywhere, Category="Audio") TSoftObjectPtr<USoundBase> SfxAlert;       // event / failure
	UPROPERTY(config, EditAnywhere, Category="Audio") TSoftObjectPtr<USoundBase> SfxTransition;  // camera + day fade

	// Helper: find stats for a crop type (falls back to first entry).
	const FCropStats& GetCrop(ECropType Type) const;
};
