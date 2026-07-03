// GreenhouseSimSubsystem.h — THE BRAIN (turn-based narrative survival).
// Manual harvest with a ripeness window; crops die from neglect (health 0) or over-ripening.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MarsSimTypes.h"
#include "GreenhouseSimSubsystem.generated.h"

class UMarsSimSettings;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSimSolEvent, int32, Sol);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSimBedEvent, int32, BedIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSimFailEvent, EResource, Cause);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSimWinEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSimEventTriggered, FEventCard, Card);

UCLASS()
class MARSGREENHOUSE_API UGreenhouseSimSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// ===== STATE =====
	UPROPERTY(BlueprintReadOnly, Category="Sim|Meters") float Oxygen = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Sim|Meters") float Water  = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Sim|Meters") float Food   = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Sim|Meters") float Power  = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="Sim") int32     Sol       = 1;
	UPROPERTY(BlueprintReadOnly, Category="Sim") float     DustLevel = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Sim") float     NextDust  = 0.f;
	UPROPERTY(BlueprintReadOnly, Category="Sim") EGameState State     = EGameState::Planning;
	UPROPERTY(BlueprintReadOnly, Category="Sim") TArray<FPlantedBed> Beds;
	UPROPERTY(BlueprintReadOnly, Category="Sim") ELedColor LedColor = ELedColor::Balanced;

	// Run stats (for the scored end-screen).
	UPROPERTY(BlueprintReadOnly, Category="Sim|Score") int32 Harvests = 0;
	UPROPERTY(BlueprintReadOnly, Category="Sim|Score") int32 Spoiled  = 0;

	UPROPERTY(BlueprintReadOnly, Category="Sim|Events") FEventCard CurrentEvent;
	UPROPERTY(BlueprintReadOnly, Category="Sim|Events") bool       bEventActive = false;
	UPROPERTY(BlueprintReadOnly, Category="Sim|Narrative") FText   CurrentFunFact;

	// ===== ACTIONS =====
	UFUNCTION(BlueprintCallable, Category="Sim") void SetLed(ELedColor NewColor);
	UFUNCTION(BlueprintCallable, Category="Sim") void CycleLed();
	UFUNCTION(BlueprintCallable, Category="Sim") void PlantCrop(int32 BedIndex, ECropType Crop);
	UFUNCTION(BlueprintCallable, Category="Sim") bool WaterPlant(int32 BedIndex);
	UFUNCTION(BlueprintCallable, Category="Sim") bool HarvestBed(int32 BedIndex); // collect / clear a spoiled crop
	UFUNCTION(BlueprintCallable, Category="Sim") bool MineIce();
	UFUNCTION(BlueprintCallable, Category="Sim") bool Electrolyze();
	UFUNCTION(BlueprintCallable, Category="Sim") void AdvanceSolNow();
	UFUNCTION(BlueprintCallable, Category="Sim") void RestartRun();
	UFUNCTION(BlueprintCallable, Category="Sim") void ResolveEvent(int32 ChoiceIndex);

	UFUNCTION(BlueprintCallable, Category="Sim") bool CanAct() const;

	// Planter status helpers (UI + world reads these).
	UFUNCTION(BlueprintCallable, Category="Sim") bool IsBedReady(int32 BedIndex) const;   // mature, not spoiled
	UFUNCTION(BlueprintCallable, Category="Sim") int32 SolsLeftToHarvest(int32 BedIndex) const; // before it spoils
	FString ResultRank() const; // "A" / "B" / "C" for the end-screen

	// ===== EVENTS =====
	UPROPERTY(BlueprintAssignable, Category="Sim|Events") FSimSolEvent  OnNewSol;
	UPROPERTY(BlueprintAssignable, Category="Sim|Events") FSimBedEvent  OnHarvest;
	UPROPERTY(BlueprintAssignable, Category="Sim|Events") FSimFailEvent OnFail;
	UPROPERTY(BlueprintAssignable, Category="Sim|Events") FSimWinEvent  OnWin;
	UPROPERTY(BlueprintAssignable, Category="Sim|Events") FSimEventTriggered OnEventTriggered;

private:
	bool bInitialized = false;
	const UMarsSimSettings* Settings = nullptr;
	float ZeroSolCounter[4] = {0,0,0,0};

	void ResetRun();
	void AdvanceSol();
	void Accumulate(float& Meter, float Delta);
	bool CheckFailAndGrace();
	float RollNewDust() const;
	void  PickFunFact();

	float DifficultyMult() const;
	float RewardBonus() const;
	float LedGrowthMult() const;
	float LedO2Mult() const;

	TArray<FName> FiredEvents;
	bool TryTriggerEvent();
	void ApplyEffect(const FEventEffect& E);
};
