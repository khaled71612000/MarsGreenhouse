// GreenhouseSimSubsystem.h — THE BRAIN.
// A UWorldSubsystem (auto lifecycle, globally reachable) that runs the closed
// life-support model on a frame-independent timer with pause-to-plan, and a
// simple game-state machine. All logic is C++; UI binds to the exposed members.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "MarsSimTypes.h"
#include "GreenhouseSimSubsystem.generated.h"

class UMarsSimSettings;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSimSolEvent, int32, Sol);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSimBedEvent, int32, BedIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSimFailEvent, EResource, Cause);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSimWinEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSimEventTriggered, FEventCard, Card);

UCLASS()
class MARSGREENHOUSE_API UGreenhouseSimSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// --- USubsystem ---
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// --- FTickableGameObject ---
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGreenhouseSimSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return bInitialized && !IsTemplate(); }
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }

	// ===== STATE (UI reads these via BindWidget bindings) =====
	UPROPERTY(BlueprintReadOnly, Category="Sim|Meters") float Oxygen   = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Sim|Meters") float Water    = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Sim|Meters") float Food     = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Sim|Meters") float Nitrogen = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Sim|Meters") float Power    = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Sim") int32      Sol       = 1;
	UPROPERTY(BlueprintReadOnly, Category="Sim") float      SolProgress = 0.f; // 0..1 within the current sol
	UPROPERTY(BlueprintReadOnly, Category="Sim") float      DustLevel = 0.f;   // 0..1 (cuts solar + light)
	UPROPERTY(BlueprintReadOnly, Category="Sim") EGameState State     = EGameState::Planning;
	UPROPERTY(BlueprintReadOnly, Category="Sim") int32      EarthCreditsLeft = 0;
	UPROPERTY(BlueprintReadOnly, Category="Sim") int32      ActionsThisSol   = 0; // acquisition actions used this sol
	UPROPERTY(BlueprintReadOnly, Category="Sim") TArray<FPlantedBed> Beds;

	// Active astronaut callout (valid while State == Event).
	UPROPERTY(BlueprintReadOnly, Category="Sim|Events") FEventCard CurrentEvent;
	UPROPERTY(BlueprintReadOnly, Category="Sim|Events") bool       bEventActive = false;

	UPROPERTY(BlueprintReadWrite, Category="Sim") bool bPaused = true; // start paused (Planning)

	// Current sourcing choices (UI sets these; default to the "safe" options).
	UPROPERTY(BlueprintReadWrite, Category="Sim|Sourcing") EWaterSource    WaterSource    = EWaterSource::Recycling;
	UPROPERTY(BlueprintReadWrite, Category="Sim|Sourcing") ELightSource    LightSource    = ELightSource::Hybrid;
	UPROPERTY(BlueprintReadWrite, Category="Sim|Sourcing") ENutrientSource NutrientSource = ENutrientSource::Compost;
	UPROPERTY(BlueprintReadWrite, Category="Sim|Sourcing") EHeatMode       HeatMode       = EHeatMode::Full;

	// ===== ACTIONS (UI buttons call these) =====
	UFUNCTION(BlueprintCallable, Category="Sim") void TogglePause();
	UFUNCTION(BlueprintCallable, Category="Sim") void WashSoil(int32 BedIndex);
	UFUNCTION(BlueprintCallable, Category="Sim") void PlantCrop(int32 BedIndex, ECropType Crop);
	UFUNCTION(BlueprintCallable, Category="Sim") void OrderNitrogenFromEarth();
	// Active acquisition (each costs power; capped at MaxActionsPerSol). Return true if it actually ran.
	UFUNCTION(BlueprintCallable, Category="Sim") bool MineIce();          // +Water, -Power
	UFUNCTION(BlueprintCallable, Category="Sim") bool Electrolyze();      // +Oxygen, -Water, -Power
	UFUNCTION(BlueprintCallable, Category="Sim") bool HarvestNitrogen();  // +Nitrogen, -Power
	UFUNCTION(BlueprintCallable, Category="Sim") bool CanAct() const;     // actions remain AND not ended
	UFUNCTION(BlueprintCallable, Category="Sim") void RestartRun();
	// ChoiceIndex: 0 = ChoiceA, 1 = ChoiceB. Called by the event popup buttons.
	UFUNCTION(BlueprintCallable, Category="Sim") void ResolveEvent(int32 ChoiceIndex);
	UFUNCTION(BlueprintCallable, Category="Sim") void AdvanceSolNow(); // resolve one sol immediately (Enter key)

	// ===== EVENTS (Blueprint binds: trigger VFX / sound / screens) =====
	UPROPERTY(BlueprintAssignable, Category="Sim|Events") FSimSolEvent  OnNewSol;
	UPROPERTY(BlueprintAssignable, Category="Sim|Events") FSimBedEvent  OnHarvest;
	UPROPERTY(BlueprintAssignable, Category="Sim|Events") FSimFailEvent OnFail;
	UPROPERTY(BlueprintAssignable, Category="Sim|Events") FSimWinEvent  OnWin;
	UPROPERTY(BlueprintAssignable, Category="Sim|Events") FSimEventTriggered OnEventTriggered;

private:
	bool   bInitialized = false;
	float  SolTimer     = 0.f;                 // counts up to SolLengthSeconds
	const UMarsSimSettings* Settings = nullptr; // cached once (game-developer: cache refs)
	float  ZeroSolCounter[5] = {0,0,0,0,0};     // grace tracking per meter (EResource order)

	void ResetRun();
	void AdvanceSol();                          // authoritative per-sol update
	void RollDust();
	float CurrentLight() const;                 // 0..1 from light source + dust
	void Accumulate(float& Meter, float Delta);
	bool CheckFailAndGrace();                   // returns true if run ended this sol

	// Event system
	TArray<FName> FiredEvents;                   // ids already used (no repeats)
	bool TryTriggerEvent();                      // returns true if a callout fired
	void ApplyEffect(const FEventEffect& E);
};
