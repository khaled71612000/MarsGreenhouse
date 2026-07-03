// GreenhouseSimSubsystem.cpp
#include "GreenhouseSimSubsystem.h"
#include "MarsSimSettings.h"

void UGreenhouseSimSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Settings = GetDefault<UMarsSimSettings>(); // cached once
	ResetRun();
	bInitialized = true;
}

void UGreenhouseSimSubsystem::ResetRun()
{
	Oxygen   = Settings->StartOxygen;
	Water    = Settings->StartWater;
	Food     = Settings->StartFood;
	Nitrogen = Settings->StartNitrogen; // low on purpose — the leak scenario
	Power    = Settings->StartPower;

	Sol = 1; SolTimer = 0.f; SolProgress = 0.f; DustLevel = 0.f;
	State = EGameState::Planning; bPaused = true;
	EarthCreditsLeft = Settings->EarthOrderCredits;
	ActionsThisSol = 0;
	for (int32 i = 0; i < 5; ++i) ZeroSolCounter[i] = 0.f;

	Beds.Reset();
	Beds.SetNum(FMath::Max(1, Settings->NumBeds)); // preallocated; no per-tick allocation later

	FiredEvents.Reset();
	bEventActive = false;
	CurrentEvent = FEventCard();
}

void UGreenhouseSimSubsystem::RestartRun() { ResetRun(); }

void UGreenhouseSimSubsystem::AdvanceSolNow()
{
	if (State == EGameState::Won || State == EGameState::Lost || State == EGameState::Event) return;
	SolTimer = 0.f;
	AdvanceSol(); // apply the whole day now instead of waiting out the timer
}

void UGreenhouseSimSubsystem::TogglePause()
{
	if (State == EGameState::Won || State == EGameState::Lost || State == EGameState::Event) return;
	bPaused = !bPaused;
	State = bPaused ? EGameState::Planning : EGameState::Running;
}

void UGreenhouseSimSubsystem::Tick(float DeltaTime)
{
	// Frame-independent: advance the sol timer only while running.
	if (bPaused || State != EGameState::Running) return;

	SolTimer += DeltaTime;
	SolProgress = FMath::Clamp(SolTimer / Settings->SolLengthSeconds, 0.f, 1.f);

	if (SolTimer >= Settings->SolLengthSeconds)
	{
		SolTimer = 0.f;
		AdvanceSol(); // apply the whole sol's deltas at once
	}
}

float UGreenhouseSimSubsystem::CurrentLight() const
{
	switch (LightSource)
	{
		case ELightSource::LED:    return 1.0f;
		case ELightSource::Hybrid: return Settings->HybridLight;
		case ELightSource::Sun:    return Settings->SunlightFraction * (1.f - DustLevel);
		default:                   return 0.f;
	}
}

void UGreenhouseSimSubsystem::Accumulate(float& Meter, float Delta)
{
	Meter = FMath::Clamp(Meter + Delta, 0.f, 100.f);
}

void UGreenhouseSimSubsystem::AdvanceSol()
{
	const UMarsSimSettings& S = *Settings;

	// --- 1. Crew consumption ---
	Accumulate(Oxygen, -S.CrewCount * S.O2PerCrew);
	Accumulate(Food,   -S.CrewCount * S.FoodPerCrew);
	Accumulate(Water,  -S.CrewCount * S.WaterPerCrew);

	// --- 2. Power generation (solar, dust-reduced) ---
	Accumulate(Power, S.SolarBasePerSol * (1.f - DustLevel));

	// --- 3. Sourcing: water ---
	if (WaterSource == EWaterSource::IceMining) { Accumulate(Water, S.IceYieldPerSol); Accumulate(Power, -S.IsruDrawPerSol); }
	else                                        { Accumulate(Water, S.RecycleYieldPerSol); Accumulate(Power, -S.RecycleDrawPerSol); }

	// --- 4. Sourcing: light power cost ---
	if (LightSource == ELightSource::LED)         Accumulate(Power, -S.LedDrawPerSol);
	else if (LightSource == ELightSource::Hybrid) Accumulate(Power, -S.LedDrawPerSol * 0.5f);

	// --- 5. Sourcing: heat ---
	Accumulate(Power, -(HeatMode == EHeatMode::Full ? S.HeaterFullPerSol : S.HeaterEcoPerSol));
	const float GrowthMult = (HeatMode == EHeatMode::Full ? 1.f : S.EcoGrowthPenalty) * S.PhotosynthesisRate;

	// --- 6. Sourcing: nutrients (nitrogen production) ---
	if (NutrientSource == ENutrientSource::Compost)       Accumulate(Nitrogen, S.CompostNitrogenPerSol);
	else if (NutrientSource == ENutrientSource::Bacteria) { Accumulate(Nitrogen, S.BacteriaNitrogenPerSol); Accumulate(Power, -S.BacteriaDrawPerSol); }
	// EarthOrder is a discrete action (OrderNitrogenFromEarth), not a per-sol drip.

	// --- 7. Crops grow (the closed loop) ---
	const float Light = CurrentLight();
	for (int32 i = 0; i < Beds.Num(); ++i)
	{
		FPlantedBed& Bed = Beds[i];
		if (!Bed.bOccupied || !Bed.bSoilClean) continue;

		const FCropStats& C = S.GetCrop(Bed.Crop);
		const float LightOK = FMath::Clamp(Light / FMath::Max(0.01f, C.LightNeed), 0.f, 1.f);

		// Health suffers if light is starved; recovers when conditions are good.
		Bed.Health = FMath::Clamp(Bed.Health + (LightOK > 0.5f ? 0.1f : -0.25f), 0.f, 1.f);

		const float Step = (1.f / FMath::Max(1.f, C.GrowSols)) * LightOK * Bed.Health * GrowthMult;
		Bed.Growth += Step;

		// Per-growth resource exchange (k scales growth-units to meter %).
		const float k = 12.f;
		Accumulate(Oxygen,   C.O2Factor    * Step * k);
		Accumulate(Water,   -C.WaterUse    * Step * k);
		Accumulate(Nitrogen,-C.NitrogenUse * Step * k); // legume NitrogenUse is negative => adds N

		if (Bed.Growth >= 1.f)
		{
			Accumulate(Food, C.FoodYield);
			OnHarvest.Broadcast(i);
			Bed = FPlantedBed(); // clear for replanting
		}
	}

	// --- 8. End-of-sol bookkeeping ---
	if (CheckFailAndGrace()) return;          // may end the run

	++Sol;
	ActionsThisSol = 0;          // refresh the player's action budget each new sol
	OnNewSol.Broadcast(Sol);

	if (Sol > S.TotalSols)                      // survived the run
	{
		State = EGameState::Won; bPaused = true;
		OnWin.Broadcast();
		return;
	}

	RollDust();                                 // set hazard for the next sol

	// Astronaut callout? If one fires, it pauses in Event state and waits for a choice.
	if (!TryTriggerEvent())
	{
		State = EGameState::Planning; bPaused = true;
	}
}

bool UGreenhouseSimSubsystem::CheckFailAndGrace()
{
	const float* Meters[5] = { &Oxygen, &Water, &Food, &Nitrogen, &Power };
	for (int32 i = 0; i < 5; ++i)
	{
		if (*Meters[i] <= 0.f)
		{
			ZeroSolCounter[i] += 1.f;
			if (ZeroSolCounter[i] > Settings->GraceSols)
			{
				State = EGameState::Lost; bPaused = true;
				OnFail.Broadcast(static_cast<EResource>(i));
				return true;
			}
		}
		else
		{
			ZeroSolCounter[i] = 0.f;
		}
	}
	return false;
}

void UGreenhouseSimSubsystem::RollDust()
{
	if (FMath::FRand() < Settings->DustChancePerSol)
		DustLevel = FMath::FRandRange(Settings->DustMagnitudeMin, Settings->DustMagnitudeMax);
	else
		DustLevel = 0.f;
}

void UGreenhouseSimSubsystem::WashSoil(int32 BedIndex)
{
	if (!Beds.IsValidIndex(BedIndex)) return;
	if (Water < Settings->SoilWashWaterCost) return;     // can't afford
	Accumulate(Water, -Settings->SoilWashWaterCost);
	Beds[BedIndex].bSoilClean = true;
}

void UGreenhouseSimSubsystem::PlantCrop(int32 BedIndex, ECropType Crop)
{
	if (!Beds.IsValidIndex(BedIndex)) return;
	FPlantedBed& Bed = Beds[BedIndex];
	if (!Bed.bSoilClean) return;                          // must wash perchlorate first
	Bed.bOccupied = true; Bed.Crop = Crop; Bed.Growth = 0.f; Bed.Health = 1.f;
}

void UGreenhouseSimSubsystem::OrderNitrogenFromEarth()
{
	if (EarthCreditsLeft <= 0) return;
	--EarthCreditsLeft;
	Accumulate(Nitrogen, Settings->EarthOrderNitrogen);
}

// ---------------- Active acquisition (recover a resource; power is the master cost) ----------------

bool UGreenhouseSimSubsystem::CanAct() const
{
	if (State == EGameState::Won || State == EGameState::Lost || State == EGameState::Event) return false;
	return ActionsThisSol < Settings->MaxActionsPerSol;
}

bool UGreenhouseSimSubsystem::MineIce()
{
	if (!CanAct()) return false;
	if (Power < Settings->MineIcePowerCost) return false;   // can't afford the power draw
	Accumulate(Power, -Settings->MineIcePowerCost);
	Accumulate(Water,  Settings->MineIceWater);
	++ActionsThisSol;
	return true;
}

bool UGreenhouseSimSubsystem::Electrolyze()
{
	if (!CanAct()) return false;
	if (Water < Settings->ElectrolyzeWaterCost || Power < Settings->ElectrolyzePowerCost) return false;
	Accumulate(Water,  -Settings->ElectrolyzeWaterCost);    // split water...
	Accumulate(Power,  -Settings->ElectrolyzePowerCost);
	Accumulate(Oxygen,  Settings->ElectrolyzeOxygen);        // ...into breathable O2
	++ActionsThisSol;
	return true;
}

bool UGreenhouseSimSubsystem::HarvestNitrogen()
{
	if (!CanAct()) return false;
	if (Power < Settings->HarvestN2PowerCost) return false;
	Accumulate(Power,   -Settings->HarvestN2PowerCost);
	Accumulate(Nitrogen, Settings->HarvestN2Nitrogen);       // pull N2 from the thin Mars air
	++ActionsThisSol;
	return true;
}

// ---------------- Event system ----------------

bool UGreenhouseSimSubsystem::TryTriggerEvent()
{
	if (FMath::FRand() > Settings->EventChancePerSol) return false;

	// Collect eligible cards (sol requirement met, not already used).
	TArray<int32> Eligible;
	for (int32 i = 0; i < Settings->Events.Num(); ++i)
	{
		const FEventCard& C = Settings->Events[i];
		if (Sol >= C.EarliestSol && !FiredEvents.Contains(C.Id))
			Eligible.Add(i);
	}
	if (Eligible.Num() == 0) return false;

	CurrentEvent = Settings->Events[Eligible[FMath::RandRange(0, Eligible.Num() - 1)]];
	FiredEvents.Add(CurrentEvent.Id);
	bEventActive = true;
	State = EGameState::Event;
	bPaused = true;
	OnEventTriggered.Broadcast(CurrentEvent);   // UI shows the popup
	return true;
}

void UGreenhouseSimSubsystem::ApplyEffect(const FEventEffect& E)
{
	Accumulate(Oxygen,   E.Oxygen);
	Accumulate(Water,    E.Water);
	Accumulate(Food,     E.Food);
	Accumulate(Nitrogen, E.Nitrogen);
	Accumulate(Power,    E.Power);
}

void UGreenhouseSimSubsystem::ResolveEvent(int32 ChoiceIndex)
{
	if (!bEventActive) return;
	ApplyEffect(ChoiceIndex == 0 ? CurrentEvent.ChoiceA.Effect : CurrentEvent.ChoiceB.Effect);
	bEventActive = false;

	// A harsh choice could zero a meter — re-check before handing control back.
	if (!CheckFailAndGrace())
	{
		State = EGameState::Planning;
		bPaused = true;
	}
}
