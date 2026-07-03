// MarsSimSettings.cpp — default crop stats + event cards (tune in Project Settings).
#include "MarsSimSettings.h"

UMarsSimSettings::UMarsSimSettings()
{
	// { Type, FoodYield, O2Factor, WaterUse, NitrogenUse, GrowSols, LightNeed }  (NitrogenUse < 0 = fixes N)
	// GrowSols = turns from plant to harvest. Timeline reflects the NASA potato life cycle
	// (8 stages: Seed Tuber -> Sprouting -> Root Dev -> Vegetative -> Tuber Initiation ->
	//  Bulking -> Maturation -> Harvest). Potato is the slow, high-yield staple; algae is the
	// fast O2 workhorse. Assign 8 stage meshes on each GreenhouseBed to show the stages.
	FCropStats Potato;  Potato.Type  = ECropType::Potato;  Potato.FoodYield  = 80.f; Potato.O2Factor = 0.8f; Potato.WaterUse = 1.0f; Potato.NitrogenUse =  1.4f; Potato.GrowSols = 12.f; Potato.LightNeed = 1.0f;
	FCropStats Lettuce; Lettuce.Type = ECropType::Lettuce; Lettuce.FoodYield = 20.f; Lettuce.O2Factor= 1.0f; Lettuce.WaterUse= 0.6f; Lettuce.NitrogenUse = 0.5f; Lettuce.GrowSols =  4.f; Lettuce.LightNeed= 0.7f;
	FCropStats Algae;   Algae.Type   = ECropType::Algae;   Algae.FoodYield   = 10.f; Algae.O2Factor  = 1.6f; Algae.WaterUse  = 0.3f; Algae.NitrogenUse  = 0.4f; Algae.GrowSols   =  2.f; Algae.LightNeed  = 0.7f;
	FCropStats Legume;  Legume.Type  = ECropType::Legume;  Legume.FoodYield  = 40.f; Legume.O2Factor = 0.9f; Legume.WaterUse = 0.9f; Legume.NitrogenUse = -1.2f; Legume.GrowSols =  8.f; Legume.LightNeed = 0.8f;
	Crops = { Potato, Lettuce, Algae, Legume };

	// ---- Event cards (bigger swings so choices matter). Add more here or in Project Settings. ----
	FEventCard E1;
	E1.Id        = FName("DustOnArray");
	E1.Speaker   = FText::FromString(TEXT("Eng. Rivera"));
	E1.Situation = FText::FromString(TEXT("Dust is choking the solar array. Send a crew out to clear it, or ride it out?"));
	E1.ChoiceA.Label  = FText::FromString(TEXT("Send a crew (risky EVA)"));
	E1.ChoiceA.Effect.Power = 18.f; E1.ChoiceA.Effect.Oxygen = -10.f;
	E1.ChoiceA.Result = FText::FromString(TEXT("Panels cleared, but the EVA burned oxygen."));
	E1.ChoiceB.Label  = FText::FromString(TEXT("Ride it out"));
	E1.ChoiceB.Effect.Power = -16.f;
	E1.ChoiceB.Result = FText::FromString(TEXT("You wait. Power drains under the dust."));
	E1.EarliestSol = 2;

	FEventCard E2;
	E2.Id        = FName("SickCrew");
	E2.Speaker   = FText::FromString(TEXT("Dr. Sato"));
	E2.Situation = FText::FromString(TEXT("A crew member is sick. Rest them and lose a pair of hands, or push the team through?"));
	E2.ChoiceA.Label  = FText::FromString(TEXT("Order rest"));
	E2.ChoiceA.Effect.Food = -8.f;
	E2.ChoiceB.Label  = FText::FromString(TEXT("Push through"));
	E2.ChoiceB.Effect.Oxygen = -8.f; E2.ChoiceB.Effect.Water = -8.f;
	E2.EarliestSol = 3;

	FEventCard E3;
	E3.Id        = FName("Resupply");
	E3.Speaker   = FText::FromString(TEXT("Mission Control"));
	E3.Situation = FText::FromString(TEXT("A small resupply pod landed. There's only room to carry one crate back."));
	E3.ChoiceA.Label  = FText::FromString(TEXT("Grab the fertilizer"));
	E3.ChoiceA.Effect.Nitrogen = 22.f;
	E3.ChoiceB.Label  = FText::FromString(TEXT("Grab the rations"));
	E3.ChoiceB.Effect.Food = 22.f;
	E3.EarliestSol = 4;

	FEventCard E4;
	E4.Id        = FName("CoolantLeak");
	E4.Speaker   = FText::FromString(TEXT("Eng. Rivera"));
	E4.Situation = FText::FromString(TEXT("A coolant line is venting. Seal it now on emergency power, or let the greenhouse run cold?"));
	E4.ChoiceA.Label  = FText::FromString(TEXT("Emergency seal"));
	E4.ChoiceA.Effect.Power = -20.f;
	E4.ChoiceB.Label  = FText::FromString(TEXT("Let it run cold"));
	E4.ChoiceB.Effect.Food = -15.f; E4.ChoiceB.Effect.Oxygen = -8.f;
	E4.EarliestSol = 3;

	Events = { E1, E2, E3, E4 };
}

const FCropStats& UMarsSimSettings::GetCrop(ECropType Type) const
{
	for (const FCropStats& C : Crops)
		if (C.Type == Type) return C;
	static const FCropStats Fallback;
	return Crops.Num() > 0 ? Crops[0] : Fallback;
}
