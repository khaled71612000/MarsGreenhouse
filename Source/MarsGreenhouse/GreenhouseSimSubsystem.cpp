// GreenhouseSimSubsystem.cpp — turn-based narrative survival brain with manual harvest + spoilage.
#include "GreenhouseSimSubsystem.h"
#include "MarsSimSettings.h"

void UGreenhouseSimSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	Settings = GetDefault<UMarsSimSettings>();
	ResetRun();
	bInitialized = true;
}

void UGreenhouseSimSubsystem::ResetRun()
{
	Oxygen = Settings->StartOxygen;
	Water  = Settings->StartWater;
	Food   = Settings->StartFood;
	Power  = Settings->StartPower;

	Sol = 1; DustLevel = 0.f; NextDust = RollNewDust();
	State = EGameState::Planning;
	LedColor = ELedColor::Balanced;
	Harvests = 0; Spoiled = 0;
	for (int32 i = 0; i < 4; ++i) ZeroSolCounter[i] = 0.f;

	Beds.Reset();
	Beds.SetNum(FMath::Max(1, Settings->NumBeds));

	FiredEvents.Reset();
	bEventActive = false;
	CurrentEvent = FEventCard();
	PickFunFact();
}

void UGreenhouseSimSubsystem::RestartRun() { ResetRun(); }

bool UGreenhouseSimSubsystem::CanAct() const { return State == EGameState::Planning; }

void UGreenhouseSimSubsystem::Accumulate(float& Meter, float Delta) { Meter = FMath::Clamp(Meter + Delta, 0.f, 100.f); }

void UGreenhouseSimSubsystem::PickFunFact()
{
	if (Settings->FunFacts.Num() > 0)
		CurrentFunFact = Settings->FunFacts[FMath::RandRange(0, Settings->FunFacts.Num() - 1)];
}

float UGreenhouseSimSubsystem::RollNewDust() const
{
	if (FMath::FRand() < Settings->DustChancePerSol)
		return FMath::FRandRange(Settings->DustMagnitudeMin, Settings->DustMagnitudeMax);
	return 0.f;
}

float UGreenhouseSimSubsystem::DifficultyMult() const
{
	if (Sol >= Settings->DiffStep2Sol) return Settings->DiffStep2Mult;
	if (Sol >= Settings->DiffStep1Sol) return Settings->DiffStep1Mult;
	return 1.f;
}

float UGreenhouseSimSubsystem::RewardBonus() const
{
	float B = 0.f;
	if (Sol >= Settings->DiffStep1Sol) B += Settings->RewardRampPerStep;
	if (Sol >= Settings->DiffStep2Sol) B += Settings->RewardRampPerStep;
	return B;
}

float UGreenhouseSimSubsystem::LedGrowthMult() const
{
	switch (LedColor)
	{
		case ELedColor::Blue: return Settings->LedGrowthBlue;
		case ELedColor::Red:  return Settings->LedGrowthRed;
		default:              return Settings->LedGrowthBalanced;
	}
}

float UGreenhouseSimSubsystem::LedO2Mult() const
{
	switch (LedColor)
	{
		case ELedColor::Blue: return Settings->LedO2Blue;
		case ELedColor::Red:  return Settings->LedO2Red;
		default:              return Settings->LedO2Balanced;
	}
}

// ---------------- light + actions ----------------

void UGreenhouseSimSubsystem::SetLed(ELedColor NewColor) { LedColor = NewColor; }
void UGreenhouseSimSubsystem::CycleLed() { LedColor = static_cast<ELedColor>(((uint8)LedColor + 1) % 3); }

void UGreenhouseSimSubsystem::PlantCrop(int32 BedIndex, ECropType Crop)
{
	if (!Beds.IsValidIndex(BedIndex) || State != EGameState::Planning) return;
	FPlantedBed& Bed = Beds[BedIndex];
	if (Bed.bOccupied) return;
	Bed = FPlantedBed();
	Bed.bOccupied = true; Bed.Crop = Crop; Bed.Health = 1.f;
}

bool UGreenhouseSimSubsystem::WaterPlant(int32 BedIndex)
{
	if (!CanAct() || !Beds.IsValidIndex(BedIndex)) return false;
	FPlantedBed& Bed = Beds[BedIndex];
	if (!Bed.bOccupied || Bed.bSpoiled) return false;
	if (Water < Settings->WaterPlantWaterCost || Power < Settings->WaterPlantPowerCost) return false;
	Accumulate(Water, -Settings->WaterPlantWaterCost);
	Accumulate(Power, -Settings->WaterPlantPowerCost);
	Bed.Health = FMath::Clamp(Bed.Health + Settings->WaterPlantHealth, 0.f, 1.f);
	return true;
}

bool UGreenhouseSimSubsystem::HarvestBed(int32 BedIndex)
{
	if (!CanAct() || !Beds.IsValidIndex(BedIndex)) return false;
	FPlantedBed& Bed = Beds[BedIndex];
	if (!Bed.bOccupied) return false;

	// Spoiled crop: clearing it frees the planter but yields nothing.
	if (Bed.bSpoiled) { Bed = FPlantedBed(); return true; }

	// Yield scales with growth (early = partial) and health (neglected = smaller).
	const FCropStats& C = Settings->GetCrop(Bed.Crop);
	const float Quality = FMath::Clamp(Bed.Growth, 0.f, 1.f) * FMath::Clamp(Bed.Health, 0.2f, 1.f);
	Accumulate(Food, C.FoodYield * Quality * (1.f + RewardBonus()));
	++Harvests;
	OnHarvest.Broadcast(BedIndex);
	Bed = FPlantedBed();
	return true;
}

bool UGreenhouseSimSubsystem::MineIce()
{
	if (!CanAct() || Power < Settings->MineIcePowerCost) return false;
	Accumulate(Power, -Settings->MineIcePowerCost);
	Accumulate(Water,  Settings->MineIceWater);
	return true;
}

bool UGreenhouseSimSubsystem::Electrolyze()
{
	if (!CanAct()) return false;
	if (Water < Settings->ElectrolyzeWaterCost || Power < Settings->ElectrolyzePowerCost) return false;
	Accumulate(Water,  -Settings->ElectrolyzeWaterCost);
	Accumulate(Power,  -Settings->ElectrolyzePowerCost);
	Accumulate(Oxygen,  Settings->ElectrolyzeOxygen);
	return true;
}

void UGreenhouseSimSubsystem::AdvanceSolNow() { if (State == EGameState::Planning) AdvanceSol(); }

// ---------------- status helpers ----------------

bool UGreenhouseSimSubsystem::IsBedReady(int32 BedIndex) const
{
	if (!Beds.IsValidIndex(BedIndex)) return false;
	const FPlantedBed& B = Beds[BedIndex];
	return B.bOccupied && !B.bSpoiled && B.Growth >= 1.f;
}

int32 UGreenhouseSimSubsystem::SolsLeftToHarvest(int32 BedIndex) const
{
	if (!IsBedReady(BedIndex)) return -1;
	const FCropStats& C = Settings->GetCrop(Beds[BedIndex].Crop);
	return FMath::Max(0, FMath::CeilToInt(C.RipeWindowSols - Beds[BedIndex].Overripe));
}

FString UGreenhouseSimSubsystem::ResultRank() const
{
	const int32 Score = Harvests * 10 - Spoiled * 6 + (State == EGameState::Won ? 25 : 0) + (Sol - 1) * 2;
	if (State == EGameState::Won && Spoiled == 0) return TEXT("S");
	if (Score >= 60) return TEXT("A");
	if (Score >= 35) return TEXT("B");
	return TEXT("C");
}

// ---------------- day resolution ----------------

void UGreenhouseSimSubsystem::AdvanceSol()
{
	const UMarsSimSettings& S = *Settings;
	const float Diff = DifficultyMult();
	const float k = S.GrowthMeterScale;

	Accumulate(Oxygen, -S.CrewCount * S.O2PerCrew   * Diff);
	Accumulate(Food,   -S.CrewCount * S.FoodPerCrew * Diff);
	Accumulate(Water,  -S.CrewCount * S.WaterPerCrew* Diff);

	Accumulate(Power, S.SolarBasePerSol * (1.f - DustLevel));
	Accumulate(Power, -S.LedDrawPerSol);
	Accumulate(Water, S.RecycleYieldPerSol);

	const float GMult  = LedGrowthMult() * S.PhotosynthesisRate;
	const float O2Mult = LedO2Mult();
	for (int32 i = 0; i < Beds.Num(); ++i)
	{
		FPlantedBed& Bed = Beds[i];
		if (!Bed.bOccupied || Bed.bSpoiled) continue;

		const FCropStats& C = S.GetCrop(Bed.Crop);
		const float Match = (C.Prefers == LedColor) ? (1.f + S.LedMatchBonus) : 1.f;
		const float Step  = (1.f / FMath::Max(1.f, C.GrowSols)) * GMult * Match * FMath::Max(0.15f, Bed.Health);

		Bed.Growth = FMath::Min(1.f, Bed.Growth + Step);
		Accumulate(Oxygen,  C.O2Factor * Step * k * O2Mult);   // keeps producing O2 while alive
		Accumulate(Water,  -C.WaterUse * Step * k);

		Bed.Health = FMath::Clamp(Bed.Health - S.HealthDecayPerSol, 0.f, 1.f);
		if (Bed.Health <= 0.f) { Bed.bSpoiled = true; ++Spoiled; continue; } // died of neglect

		if (Bed.Growth >= 1.f)
		{
			Bed.Overripe += 1.f;
			if (Bed.Overripe > C.RipeWindowSols) { Bed.bSpoiled = true; ++Spoiled; } // bolted / rotted
		}
	}

	if (CheckFailAndGrace()) return;

	++Sol;
	OnNewSol.Broadcast(Sol);

	if (Sol > S.TotalSols) { State = EGameState::Won; OnWin.Broadcast(); return; }

	DustLevel = NextDust;
	NextDust  = RollNewDust();
	PickFunFact();

	if (!TryTriggerEvent()) State = EGameState::Planning;
}

bool UGreenhouseSimSubsystem::CheckFailAndGrace()
{
	float* Meters[4] = { &Oxygen, &Water, &Food, &Power };
	for (int32 i = 0; i < 4; ++i)
	{
		if (*Meters[i] <= 0.f)
		{
			ZeroSolCounter[i] += 1.f;
			if (ZeroSolCounter[i] > Settings->GraceSols)
			{
				State = EGameState::Lost;
				OnFail.Broadcast(static_cast<EResource>(i));
				return true;
			}
		}
		else ZeroSolCounter[i] = 0.f;
	}
	return false;
}

// ---------------- events / narrative ----------------

bool UGreenhouseSimSubsystem::TryTriggerEvent()
{
	for (int32 i = 0; i < Settings->Events.Num(); ++i)
	{
		const FEventCard& C = Settings->Events[i];
		if (C.ScriptedSol == Sol && !FiredEvents.Contains(C.Id))
		{
			CurrentEvent = C; FiredEvents.Add(C.Id); bEventActive = true;
			State = EGameState::Event; OnEventTriggered.Broadcast(CurrentEvent); return true;
		}
	}

	if (FMath::FRand() > Settings->EventChancePerSol) return false;

	TArray<int32> Eligible;
	for (int32 i = 0; i < Settings->Events.Num(); ++i)
	{
		const FEventCard& C = Settings->Events[i];
		if (C.ScriptedSol == 0 && Sol >= C.EarliestSol && !FiredEvents.Contains(C.Id)) Eligible.Add(i);
	}
	if (Eligible.Num() == 0) return false;

	CurrentEvent = Settings->Events[Eligible[FMath::RandRange(0, Eligible.Num() - 1)]];
	FiredEvents.Add(CurrentEvent.Id); bEventActive = true;
	State = EGameState::Event; OnEventTriggered.Broadcast(CurrentEvent);
	return true;
}

void UGreenhouseSimSubsystem::ApplyEffect(const FEventEffect& E)
{
	Accumulate(Oxygen, E.Oxygen);
	Accumulate(Water,  E.Water);
	Accumulate(Food,   E.Food);
	Accumulate(Power,  E.Power);
}

void UGreenhouseSimSubsystem::ResolveEvent(int32 ChoiceIndex)
{
	if (!bEventActive) return;
	if (!CurrentEvent.bSingleChoice || ChoiceIndex == 0)
		ApplyEffect(ChoiceIndex == 0 ? CurrentEvent.ChoiceA.Effect : CurrentEvent.ChoiceB.Effect);
	bEventActive = false;
	if (!CheckFailAndGrace()) State = EGameState::Planning;
}
