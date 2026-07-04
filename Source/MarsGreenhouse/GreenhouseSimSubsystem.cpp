// GreenhouseSimSubsystem.cpp — turn-based survival brain (Bustan): manual harvest, spoilage,
// mission log, crew status, dynamic objective, Purple/White grow-light.
#include "GreenhouseSimSubsystem.h"
#include "MarsSimSettings.h"
#include "GreenhouseBed.h"
#include "Kismet/GameplayStatics.h"

static const TCHAR* ResName(EResource R)
{
	switch (R) { case EResource::Oxygen: return TEXT("Oxygen"); case EResource::Water: return TEXT("Water"); case EResource::Food: return TEXT("Food"); default: return TEXT("Power"); }
}

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
	LedColor = ELedColor::Purple;
	Harvests = 0; Spoiled = 0;
	for (int32 i = 0; i < 4; ++i) ZeroSolCounter[i] = 0.f;

	Beds.Reset();
	Beds.SetNum(FMath::Max(1, Settings->NumBeds));

	FiredEvents.Reset();
	bEventActive = false;
	CurrentEvent = FEventCard();
	MissionLog.Reset();
	Log(TEXT("Bustan online. Sol 1 begins."));
	PickFunFact();
	SyncBedsFromWorld();
}

void UGreenhouseSimSubsystem::RestartRun() { ResetRun(); }

void UGreenhouseSimSubsystem::SyncBedsFromWorld()
{
	if (!GetWorld()) return;
	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AGreenhouseBed::StaticClass(), Found);
	int32 MaxIdx = -1;
	for (AActor* A : Found)
		if (const AGreenhouseBed* B = Cast<AGreenhouseBed>(A))
			MaxIdx = FMath::Max(MaxIdx, B->BedIndex);
	const int32 Floor = FMath::Max(1, Settings ? Settings->NumBeds : 1);
	const int32 Need  = FMath::Max(Floor, MaxIdx + 1);
	if (Beds.Num() < Need) Beds.SetNum(Need); // grow only; never drop an occupied bed
}

bool UGreenhouseSimSubsystem::CanAct() const { return State == EGameState::Planning; }

void UGreenhouseSimSubsystem::Accumulate(float& Meter, float Delta) { Meter = FMath::Clamp(Meter + Delta, 0.f, 100.f); }

void UGreenhouseSimSubsystem::Log(const FString& Line)
{
	MissionLog.Add(FString::Printf(TEXT("Sol %d  -  %s"), Sol, *Line));
	while (MissionLog.Num() > 12) MissionLog.RemoveAt(0);
}

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
	return (LedColor == ELedColor::White) ? Settings->LedGrowthWhite : Settings->LedGrowthPurple;
}

float UGreenhouseSimSubsystem::LedO2Mult() const
{
	return (LedColor == ELedColor::White) ? Settings->LedO2White : Settings->LedO2Purple;
}

// ---------------- light + actions ----------------

void UGreenhouseSimSubsystem::SetLed(ELedColor NewColor) { LedColor = NewColor; }
void UGreenhouseSimSubsystem::CycleLed() { LedColor = (LedColor == ELedColor::Purple) ? ELedColor::White : ELedColor::Purple; }

void UGreenhouseSimSubsystem::PlantCrop(int32 BedIndex, ECropType Crop)
{
	if (!Beds.IsValidIndex(BedIndex) || State != EGameState::Planning) return;
	FPlantedBed& Bed = Beds[BedIndex];
	if (Bed.bOccupied) return;
	Bed = FPlantedBed();
	Bed.bOccupied = true; Bed.Crop = Crop; Bed.Health = 1.f;
	Log(FString::Printf(TEXT("Planted %s in Planter %d."), Crop == ECropType::Potato ? TEXT("potato") : TEXT("lettuce"), BedIndex));
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

	if (Bed.bSpoiled) { Bed = FPlantedBed(); Log(FString::Printf(TEXT("Cleared spoiled crop from Planter %d."), BedIndex)); return true; }

	const FCropStats& C = Settings->GetCrop(Bed.Crop);
	const float Quality = FMath::Clamp(Bed.Growth, 0.f, 1.f) * FMath::Clamp(Bed.Health, 0.2f, 1.f);
	const int32 Yield = FMath::RoundToInt(C.FoodYield * Quality * (1.f + RewardBonus()));
	Accumulate(Food, (float)Yield);
	++Harvests;
	OnHarvest.Broadcast(BedIndex);
	Log(FString::Printf(TEXT("Harvested %s from Planter %d (+%d food)."), Bed.Crop == ECropType::Potato ? TEXT("potato") : TEXT("lettuce"), BedIndex, Yield));
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

FString UGreenhouseSimSubsystem::GetCrewStatus(int32 Index) const
{
	if (State == EGameState::Lost) return TEXT("Critical");
	const float Lowest = FMath::Min(FMath::Min(Oxygen, Water), FMath::Min(Food, Power));
	if (Lowest < 20.f && (Index % 2 == 0)) return TEXT("Strained");
	if (Food < 30.f && (Index % 3 == 0))   return TEXT("Tired");
	return TEXT("OK");
}

FString UGreenhouseSimSubsystem::ResultRank() const
{
	const int32 Score = Harvests * 10 - Spoiled * 6 + (State == EGameState::Won ? 25 : 0) + (Sol - 1) * 2;
	if (State == EGameState::Won && Spoiled == 0) return TEXT("S");
	if (Score >= 60) return TEXT("A");
	if (Score >= 35) return TEXT("B");
	return TEXT("C");
}

FString UGreenhouseSimSubsystem::CurrentObjective() const
{
	if (State == EGameState::Won)  return TEXT("Bustan survived all 15 sols. Well done, Commander.");
	if (State == EGameState::Lost) return TEXT("The garden fell. Press Restart to try again.");
	if (Power  <= 20.f) return TEXT("Power critical - spend actions sparingly and pass the sol.");
	if (Oxygen <= 20.f) return TEXT("Oxygen low - electrolyze water, or grow lettuce under red + blue light.");
	if (Food   <= 20.f) return TEXT("Food low - harvest a ready crop or plant fast lettuce.");
	int32 Occ = 0, Ready = 0;
	for (int32 i = 0; i < Beds.Num(); ++i) { if (Beds[i].bOccupied) ++Occ; if (IsBedReady(i)) ++Ready; }
	if (Ready > 0) return TEXT("A crop is ready - harvest it before it spoils.");
	if (Occ == 0)  return TEXT("Plant your first crop in a planter to start growing food.");
	return Settings->Objective.ToString();
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
		Accumulate(Oxygen,  C.O2Factor * Step * k * O2Mult);
		Accumulate(Water,  -C.WaterUse * Step * k);

		Bed.Health = FMath::Clamp(Bed.Health - S.HealthDecayPerSol, 0.f, 1.f);
		if (Bed.Health <= 0.f) { Bed.bSpoiled = true; ++Spoiled; Log(FString::Printf(TEXT("Planter %d crop died of neglect."), i)); continue; }

		if (Bed.Growth >= 1.f)
		{
			Bed.Overripe += 1.f;
			if (Bed.Overripe > C.RipeWindowSols) { Bed.bSpoiled = true; ++Spoiled; Log(FString::Printf(TEXT("Planter %d crop spoiled (%s)."), i, *C.SpoilName.ToString())); }
		}
	}

	if (CheckFailAndGrace()) return;

	++Sol;
	OnNewSol.Broadcast(Sol);
	Log(FString::Printf(TEXT("Sol %d begins."), Sol));

	if (Sol > S.TotalSols) { State = EGameState::Won; Log(TEXT("All sols survived - Bustan endures.")); OnWin.Broadcast(); return; }

	DustLevel = NextDust;
	NextDust  = RollNewDust();
	if (DustLevel > 0.f) Log(FString::Printf(TEXT("Dust storm - solar down %d%%."), FMath::RoundToInt(DustLevel*100.f)));
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
				Log(FString::Printf(TEXT("%s depleted - colony lost."), ResName(static_cast<EResource>(i))));
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
			State = EGameState::Event; Log(FString::Printf(TEXT("%s: decision required."), *C.Speaker.ToString()));
			OnEventTriggered.Broadcast(CurrentEvent); return true;
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
	State = EGameState::Event; Log(FString::Printf(TEXT("%s: decision required."), *CurrentEvent.Speaker.ToString()));
	OnEventTriggered.Broadcast(CurrentEvent);
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
