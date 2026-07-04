// MarsSimSettings.h — all tunable numbers + assignable art, in Project Settings -> Game -> "Mars Greenhouse Sim".
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MarsSimTypes.h"
#include "MarsSimSettings.generated.h"

class UUserWidget;
class UEventPopupWidget;
class USoundBase;
class UFont;
class UTexture2D;

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

	// --- Crew consumption (per sol, in meter units) ---
	UPROPERTY(config, EditAnywhere, Category="Crew")  int32 CrewCount   = 6;
	UPROPERTY(config, EditAnywhere, Category="Crew")  int32 GraceSols    = 1;
	UPROPERTY(config, EditAnywhere, Category="Crew")  float O2PerCrew    = 2.2f;
	UPROPERTY(config, EditAnywhere, Category="Crew")  float FoodPerCrew  = 1.3f;
	UPROPERTY(config, EditAnywhere, Category="Crew")  float WaterPerCrew = 1.1f;

	// --- Starting meters ---
	UPROPERTY(config, EditAnywhere, Category="Start") float StartOxygen = 60.f;
	UPROPERTY(config, EditAnywhere, Category="Start") float StartWater  = 70.f;
	UPROPERTY(config, EditAnywhere, Category="Start") float StartFood   = 55.f;
	UPROPERTY(config, EditAnywhere, Category="Start") float StartPower  = 65.f;

	// --- Power / Water ---
	UPROPERTY(config, EditAnywhere, Category="Power") float SolarBasePerSol = 40.f;
	UPROPERTY(config, EditAnywhere, Category="Power") float LedDrawPerSol   = 9.f;
	UPROPERTY(config, EditAnywhere, Category="Water") float RecycleYieldPerSol = 10.f;

	// --- Growth ---
	UPROPERTY(config, EditAnywhere, Category="Growth") float PhotosynthesisRate = 1.0f;
	UPROPERTY(config, EditAnywhere, Category="Growth") float GrowthMeterScale   = 12.f;
	UPROPERTY(config, EditAnywhere, Category="Growth") float HealthDecayPerSol  = 0.10f;

	// --- Grow-light (Purple = efficient/O2, White = fast growth) ---
	UPROPERTY(config, EditAnywhere, Category="Light") float LedGrowthPurple = 1.00f;
	UPROPERTY(config, EditAnywhere, Category="Light") float LedGrowthWhite  = 1.20f;
	UPROPERTY(config, EditAnywhere, Category="Light") float LedO2Purple     = 1.25f;
	UPROPERTY(config, EditAnywhere, Category="Light") float LedO2White      = 0.90f;
	UPROPERTY(config, EditAnywhere, Category="Light") float LedMatchBonus   = 0.15f;

	// --- Actions ---
	UPROPERTY(config, EditAnywhere, Category="Actions") float MineIceWater         = 18.f;
	UPROPERTY(config, EditAnywhere, Category="Actions") float MineIcePowerCost     = 12.f;
	UPROPERTY(config, EditAnywhere, Category="Actions") float ElectrolyzeOxygen    = 14.f;
	UPROPERTY(config, EditAnywhere, Category="Actions") float ElectrolyzeWaterCost = 10.f;
	UPROPERTY(config, EditAnywhere, Category="Actions") float ElectrolyzePowerCost = 8.f;
	UPROPERTY(config, EditAnywhere, Category="Actions") float WaterPlantWaterCost  = 6.f;
	UPROPERTY(config, EditAnywhere, Category="Actions") float WaterPlantPowerCost  = 2.f;
	UPROPERTY(config, EditAnywhere, Category="Actions") float WaterPlantHealth     = 0.35f;

	// --- Dust + difficulty ---
	UPROPERTY(config, EditAnywhere, Category="Dust") float DustChancePerSol = 0.18f;
	UPROPERTY(config, EditAnywhere, Category="Dust") float DustMagnitudeMin = 0.40f;
	UPROPERTY(config, EditAnywhere, Category="Dust") float DustMagnitudeMax = 0.70f;
	UPROPERTY(config, EditAnywhere, Category="Difficulty") int32 DiffStep1Sol = 5;
	UPROPERTY(config, EditAnywhere, Category="Difficulty") int32 DiffStep2Sol = 10;
	UPROPERTY(config, EditAnywhere, Category="Difficulty") float DiffStep1Mult = 1.25f;
	UPROPERTY(config, EditAnywhere, Category="Difficulty") float DiffStep2Mult = 1.50f;
	UPROPERTY(config, EditAnywhere, Category="Difficulty") float RewardRampPerStep = 0.20f;

	// --- Crops / events / narrative ---
	UPROPERTY(config, EditAnywhere, Category="Crops") TArray<FCropStats> Crops;
	UPROPERTY(config, EditAnywhere, Category="Events") float EventChancePerSol = 0.4f;
	UPROPERTY(config, EditAnywhere, Category="Events") TArray<FEventCard> Events;
	UPROPERTY(config, EditAnywhere, Category="Narrative") TArray<FText> FunFacts;
	UPROPERTY(config, EditAnywhere, Category="Narrative") FText Objective;
	UPROPERTY(config, EditAnywhere, Category="Narrative") TArray<FCrewMember> Crew;

	// ============ ASSIGNABLE ART (all optional; empty = code-drawn fallback) ============
	UPROPERTY(config, EditAnywhere, Category="Art|Font")  TSoftObjectPtr<UFont> HudFont;

	UPROPERTY(config, EditAnywhere, Category="Art|Icons") TSoftObjectPtr<UTexture2D> IconOxygen;
	UPROPERTY(config, EditAnywhere, Category="Art|Icons") TSoftObjectPtr<UTexture2D> IconWater;
	UPROPERTY(config, EditAnywhere, Category="Art|Icons") TSoftObjectPtr<UTexture2D> IconFood;
	UPROPERTY(config, EditAnywhere, Category="Art|Icons") TSoftObjectPtr<UTexture2D> IconPower;

	UPROPERTY(config, EditAnywhere, Category="Art|Icons") TSoftObjectPtr<UTexture2D> IconPlant;
	UPROPERTY(config, EditAnywhere, Category="Art|Icons") TSoftObjectPtr<UTexture2D> IconWaterAction;
	UPROPERTY(config, EditAnywhere, Category="Art|Icons") TSoftObjectPtr<UTexture2D> IconHarvest;
	UPROPERTY(config, EditAnywhere, Category="Art|Icons") TSoftObjectPtr<UTexture2D> IconMineIce;
	UPROPERTY(config, EditAnywhere, Category="Art|Icons") TSoftObjectPtr<UTexture2D> IconElectrolyze;
	UPROPERTY(config, EditAnywhere, Category="Art|Icons") TSoftObjectPtr<UTexture2D> IconEndDay;

	UPROPERTY(config, EditAnywhere, Category="Art|Icons") TSoftObjectPtr<UTexture2D> IconLight;
	UPROPERTY(config, EditAnywhere, Category="Art|Icons") TSoftObjectPtr<UTexture2D> IconStorm;
	UPROPERTY(config, EditAnywhere, Category="Art|Icons") TSoftObjectPtr<UTexture2D> IconAlert;

	UPROPERTY(config, EditAnywhere, Category="Art|Frames") TSoftObjectPtr<UTexture2D> PanelFrame;
	UPROPERTY(config, EditAnywhere, Category="Art|Frames") float PanelMargin = 24.f; // corner size of the 9-slice
	UPROPERTY(config, EditAnywhere, Category="Art|Frames") TSoftObjectPtr<UTexture2D> ButtonFrame;
	UPROPERTY(config, EditAnywhere, Category="Art|Frames") float ButtonMargin = 12.f;
	UPROPERTY(config, EditAnywhere, Category="Art|Frames") TSoftObjectPtr<UTexture2D> MeterBg;
	UPROPERTY(config, EditAnywhere, Category="Art|Frames") TSoftObjectPtr<UTexture2D> MeterFill;
	UPROPERTY(config, EditAnywhere, Category="Art|Frames") TSoftObjectPtr<UTexture2D> CameraFrame;

	UPROPERTY(config, EditAnywhere, Category="Art|Misc") TSoftObjectPtr<UTexture2D> TitleLogo;
	UPROPERTY(config, EditAnywhere, Category="Art|Misc") TSoftObjectPtr<UTexture2D> VignetteOverlay;
	UPROPERTY(config, EditAnywhere, Category="Art|Misc") TArray<TSoftObjectPtr<UTexture2D>> CrewPortraits; // one per Crew entry

	// --- Optional custom UMG (leave empty to use the built-in C++ HUD) ---
	UPROPERTY(config, EditAnywhere, Category="UI") TSoftClassPtr<UUserWidget> DashboardWidgetClass;
	UPROPERTY(config, EditAnywhere, Category="UI") TSoftClassPtr<UEventPopupWidget> EventPopupWidgetClass;

	// --- Optional audio ---
	UPROPERTY(config, EditAnywhere, Category="Audio") TSoftObjectPtr<USoundBase> MusicLoop;
	UPROPERTY(config, EditAnywhere, Category="Audio") TSoftObjectPtr<USoundBase> SfxClick;
	UPROPERTY(config, EditAnywhere, Category="Audio") TSoftObjectPtr<USoundBase> SfxConfirm;
	UPROPERTY(config, EditAnywhere, Category="Audio") TSoftObjectPtr<USoundBase> SfxAlert;
	UPROPERTY(config, EditAnywhere, Category="Audio") TSoftObjectPtr<USoundBase> SfxTransition;

	const FCropStats& GetCrop(ECropType Type) const;
};
