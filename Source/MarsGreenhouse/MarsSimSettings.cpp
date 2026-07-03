// MarsSimSettings.cpp — default crops, the narrative arc + random events, and day-start fun facts.
#include "MarsSimSettings.h"

UMarsSimSettings::UMarsSimSettings()
{
	// ---- Crops (2). Potato = slow high-yield staple, prefers RED (tuber bulking).
	//      Lettuce = fast, good O2, prefers BLUE (leafy). ----
	FCropStats Potato;  Potato.Type  = ECropType::Potato;  Potato.FoodYield  = 80.f; Potato.O2Factor = 0.9f; Potato.WaterUse = 1.0f; Potato.GrowSols = 10.f; Potato.Prefers = ELedColor::Red;
	Potato.StageEnds  = { 0.19f, 0.37f, 0.86f, 1.0f };
	Potato.StageNames = { FText::FromString(TEXT("Sprouting")), FText::FromString(TEXT("Stolons")), FText::FromString(TEXT("Tuber bulking")), FText::FromString(TEXT("Ready")) };
	Potato.RipeWindowSols = 4.f; Potato.SpoilName = FText::FromString(TEXT("Rotting"));

	FCropStats Lettuce; Lettuce.Type = ECropType::Lettuce; Lettuce.FoodYield = 24.f; Lettuce.O2Factor= 1.2f; Lettuce.WaterUse= 0.6f; Lettuce.GrowSols =  4.f; Lettuce.Prefers = ELedColor::Blue;
	Lettuce.StageEnds  = { 0.20f, 0.45f, 0.80f, 1.0f };
	Lettuce.StageNames = { FText::FromString(TEXT("Seed")), FText::FromString(TEXT("Germination")), FText::FromString(TEXT("Vegetative")), FText::FromString(TEXT("Ready")) };
	Lettuce.RipeWindowSols = 2.f; Lettuce.SpoilName = FText::FromString(TEXT("Bolting"));

	Crops = { Potato, Lettuce };

	auto Single = [](FEventCard& C, const TCHAR* Continue)
	{
		C.bSingleChoice = true;
		C.ChoiceA.Label = FText::FromString(Continue);
	};

	// ================= NARRATIVE ARC (scripted story beats) =================
	FEventCard B3;
	B3.Id = FName("Beat_Alone"); B3.ScriptedSol = 3;
	B3.Speaker   = FText::FromString(TEXT("Mission Control"));
	B3.Situation = FText::FromString(TEXT("Transmission from Earth: the next resupply window is 200 sols out. Whatever you grow now is all you'll eat. Keep the greenhouse alive."));
	Single(B3, TEXT("Understood. We hold."));

	FEventCard B6;
	B6.Id = FName("Beat_Micrometeorite"); B6.ScriptedSol = 6;
	B6.Speaker   = FText::FromString(TEXT("Eng. Rivera"));
	B6.Situation = FText::FromString(TEXT("A micrometeorite cracked an outer panel. Dust is seeping toward the array. Seal it on emergency power, or pull the crew off farm duty to patch it by hand?"));
	B6.ChoiceA.Label = FText::FromString(TEXT("Emergency seal (burn power)"));
	B6.ChoiceA.Effect.Power = -16.f;
	B6.ChoiceB.Label = FText::FromString(TEXT("Crew patches it (skip a harvest meal)"));
	B6.ChoiceB.Effect.Food = -12.f;

	FEventCard B9;
	B9.Id = FName("Beat_Recycler"); B9.ScriptedSol = 9;
	B9.Speaker   = FText::FromString(TEXT("Dr. Sato"));
	B9.Situation = FText::FromString(TEXT("The water recycler is failing. Cannibalize a spare unit for parts, or run it degraded and lose some water each sol until we can fix it?"));
	B9.ChoiceA.Label = FText::FromString(TEXT("Cannibalize spare (costs power)"));
	B9.ChoiceA.Effect.Power = -14.f;
	B9.ChoiceB.Label = FText::FromString(TEXT("Run degraded (lose water now)"));
	B9.ChoiceB.Effect.Water = -18.f;

	FEventCard B12;
	B12.Id = FName("Beat_StormWarning"); B12.ScriptedSol = 12;
	B12.Speaker   = FText::FromString(TEXT("Mission Control"));
	B12.Situation = FText::FromString(TEXT("Orbital imaging shows a regional dust storm closing in. It will choke the solar array for days. Bank power now by rationing, or gamble that it misses us?"));
	B12.ChoiceA.Label = FText::FromString(TEXT("Ration and bank power"));
	B12.ChoiceA.Effect.Food = -6.f; B12.ChoiceA.Effect.Power = 14.f;
	B12.ChoiceB.Label = FText::FromString(TEXT("Gamble it misses"));
	B12.ChoiceB.Effect.Power = -4.f;

	FEventCard B14;
	B14.Id = FName("Beat_StormHits"); B14.ScriptedSol = 14;
	B14.Speaker   = FText::FromString(TEXT("Cmdr. Okafor"));
	B14.Situation = FText::FromString(TEXT("The storm is on top of us. Sky's gone red, solar's crawling. One more sol. Hold the line and bring the colony home."));
	Single(B14, TEXT("Hold the line."));

	// ================= RANDOM POOL =================
	FEventCard R1;
	R1.Id = FName("DustOnArray"); R1.EarliestSol = 2;
	R1.Speaker   = FText::FromString(TEXT("Eng. Rivera"));
	R1.Situation = FText::FromString(TEXT("Dust is dulling the solar array. Send a crew out to clear it, or ride it out?"));
	R1.ChoiceA.Label = FText::FromString(TEXT("Clear it (risky EVA)"));
	R1.ChoiceA.Effect.Power = 16.f; R1.ChoiceA.Effect.Oxygen = -8.f;
	R1.ChoiceB.Label = FText::FromString(TEXT("Ride it out"));
	R1.ChoiceB.Effect.Power = -12.f;

	FEventCard R2;
	R2.Id = FName("SickCrew"); R2.EarliestSol = 3;
	R2.Speaker   = FText::FromString(TEXT("Dr. Sato"));
	R2.Situation = FText::FromString(TEXT("A crew member is sick. Rest them and lose a pair of hands, or push the team through?"));
	R2.ChoiceA.Label = FText::FromString(TEXT("Order rest"));
	R2.ChoiceA.Effect.Food = -8.f;
	R2.ChoiceB.Label = FText::FromString(TEXT("Push through"));
	R2.ChoiceB.Effect.Oxygen = -6.f; R2.ChoiceB.Effect.Water = -6.f;

	FEventCard R3;
	R3.Id = FName("Condensation"); R3.EarliestSol = 2;
	R3.Speaker   = FText::FromString(TEXT("Dr. Sato"));
	R3.Situation = FText::FromString(TEXT("Overnight condensation pooled on the dome. Harvest the clean water, or vent it to keep humidity down for the crops?"));
	R3.ChoiceA.Label = FText::FromString(TEXT("Collect the water"));
	R3.ChoiceA.Effect.Water = 16.f;
	R3.ChoiceB.Label = FText::FromString(TEXT("Vent it (protect plants)"));
	R3.ChoiceB.Effect.Oxygen = 8.f;

	FEventCard R4;
	R4.Id = FName("LedSurge"); R4.EarliestSol = 4;
	R4.Speaker   = FText::FromString(TEXT("Eng. Rivera"));
	R4.Situation = FText::FromString(TEXT("The grow-lights can be overdriven for a burst. Push them for extra photosynthesis, or spare the power?"));
	R4.ChoiceA.Label = FText::FromString(TEXT("Overdrive the lights"));
	R4.ChoiceA.Effect.Oxygen = 12.f; R4.ChoiceA.Effect.Power = -12.f;
	R4.ChoiceB.Label = FText::FromString(TEXT("Spare the power"));
	R4.ChoiceB.Effect.Power = 4.f;

	FEventCard R5;
	R5.Id = FName("MoraleFeast"); R5.EarliestSol = 5;
	R5.Speaker   = FText::FromString(TEXT("Cmdr. Okafor"));
	R5.Situation = FText::FromString(TEXT("Morale is low. Spend some rations on a proper meal to lift the crew, or stay lean?"));
	R5.ChoiceA.Label = FText::FromString(TEXT("Host a meal"));
	R5.ChoiceA.Effect.Food = -10.f; R5.ChoiceA.Effect.Oxygen = 6.f;
	R5.ChoiceB.Label = FText::FromString(TEXT("Stay lean"));
	R5.ChoiceB.Effect.Food = 2.f;

	Events = { B3, B6, B9, B12, B14, R1, R2, R3, R4, R5 };

	// ================= FUN FACTS (accurate) =================
	FunFacts = {
		FText::FromString(TEXT("Mars receives only ~43% of Earth's sunlight, so Martian greenhouses lean hard on LED grow-lights.")),
		FText::FromString(TEXT("Blue light boosts chlorophyll and oxygen output; red light speeds growth and swells potato tubers.")),
		FText::FromString(TEXT("A Martian sol is 24 hours 39 minutes - just a little longer than an Earth day.")),
		FText::FromString(TEXT("Mars soil holds perchlorates that are toxic to humans, so crops are grown in treated or hydroponic beds.")),
		FText::FromString(TEXT("NASA studied potatoes for space farming: dense calories per square meter of greenhouse.")),
		FText::FromString(TEXT("The Martian atmosphere is ~95% CO2 - great for photosynthesis, useless for breathing.")),
		FText::FromString(TEXT("Water on Mars is locked in subsurface ice; extracting it costs precious energy.")),
		FText::FromString(TEXT("Electrolysis splits water into breathable oxygen and hydrogen fuel.")),
		FText::FromString(TEXT("Global dust storms can blanket Mars for weeks, cutting solar power to a trickle.")),
		FText::FromString(TEXT("Lettuce grows fast and freshens the air - a favorite crop aboard the ISS.")),
		FText::FromString(TEXT("Mars gravity is about one-third of Earth's, changing how water and roots move in soil.")),
		FText::FromString(TEXT("A sealed greenhouse recycles nearly all its water through plant transpiration and condensation."))
	};
}

const FCropStats& UMarsSimSettings::GetCrop(ECropType Type) const
{
	for (const FCropStats& C : Crops)
		if (C.Type == Type) return C;
	static const FCropStats Fallback;
	return Crops.Num() > 0 ? Crops[0] : Fallback;
}
