// Copyright Epic Games, Inc. All Rights Reserved.
// Fully click-driven HUD. Every interactive element registers a hit box whose
// FName is routed to AMarsGreenhousePlayerController::HandleHudAction. Cameras
// switch by clicking their thumbnails (no keys). A skippable guided tutorial
// highlights the element the player should use next.

#include "MarsGreenhouseHUD.h"
#include "GreenhouseSimSubsystem.h"
#include "MarsSimSettings.h"
#include "MarsGreenhousePlayerController.h"
#include "GreenhouseCamera.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void AMarsGreenhouseHUD::NotifyHitBoxClick(FName BoxName)
{
	Super::NotifyHitBoxClick(BoxName);
	if (AMarsGreenhousePlayerController* PC = Cast<AMarsGreenhousePlayerController>(PlayerOwner))
		PC->HandleHudAction(BoxName);
}

void AMarsGreenhouseHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;

	UWorld* W = GetWorld();
	UGreenhouseSimSubsystem* S = W ? W->GetSubsystem<UGreenhouseSimSubsystem>() : nullptr;
	if (!S) return;

	const UMarsSimSettings* Cfg = GetDefault<UMarsSimSettings>();
	UFont* Font = GEngine ? GEngine->GetMediumFont() : nullptr;
	AMarsGreenhousePlayerController* PC = Cast<AMarsGreenhousePlayerController>(PlayerOwner);
	const bool bCustomDash  = PC && PC->HasCustomDashboard();
	const bool bCustomEvent = PC && PC->HasCustomEventUI();

	const FLinearColor White (0.93f, 0.95f, 0.99f);
	const FLinearColor Grey  (0.62f, 0.68f, 0.80f);
	const FLinearColor Faint (0.42f, 0.48f, 0.60f);
	const FLinearColor Cyan  (0.36f, 0.82f, 0.90f);
	const FLinearColor Amber (0.97f, 0.72f, 0.28f);
	const FLinearColor Green (0.40f, 0.84f, 0.50f);
	const FLinearColor Red   (0.96f, 0.36f, 0.36f);
	const FLinearColor Orange(1.00f, 0.48f, 0.29f);
	const FLinearColor Panel (0.03f, 0.05f, 0.11f, 0.88f);
	const FLinearColor Line  (0.16f, 0.22f, 0.34f, 1.f);

	const float SW = Canvas->SizeX, SH = Canvas->SizeY;
	const float Now = W->GetTimeSeconds();
	const int32 TStep = (PC && PC->TutorialActive()) ? PC->TutorialStepIndex() : -1;

	// highlight box for the tutorial (set while drawing the target element)
	bool bHL = false; float hlx=0, hly=0, hlw=0, hlh=0;
	auto SetHL = [&](int32 forStep, float x, float y, float w, float h){ if (TStep == forStep){ bHL=true; hlx=x; hly=y; hlw=w; hlh=h; } };

	// ---- reusable drawing helpers ----
	auto Divider = [&](float x, float y, float w){ DrawRect(Line, x, y, w, 1.f); };

	auto Outline = [&](float x, float y, float w, float h, const FLinearColor& c, float t){
		DrawRect(c, x, y, w, t); DrawRect(c, x, y+h-t, w, t);
		DrawRect(c, x, y, t, h); DrawRect(c, x+w-t, y, t, h);
	};

	auto Button = [&](float x, float y, float w, float h, const FString& label, const FString& sub, const FLinearColor& fill, FName id, bool enabled){
		const FLinearColor bg = enabled ? fill : FLinearColor(0.11f, 0.13f, 0.19f, 0.92f);
		DrawRect(bg, x, y, w, h);
		DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.06f), x, y, w, 2.f);
		DrawText(label, enabled ? White : Faint, x + 10.f, y + (sub.IsEmpty() ? h*0.5f - 8.f : 6.f), Font);
		if (!sub.IsEmpty()) DrawText(sub, enabled ? Grey : Faint, x + 10.f, y + h - 19.f, Font, 0.82f);
		if (enabled) AddHitBox(FVector2D(x, y), FVector2D(w, h), id, true);
	};

	auto Chip = [&](float x, float y, float w, const FString& label, const FString& val, FName id){
		const float h = 40.f;
		DrawRect(FLinearColor(0.07f, 0.10f, 0.18f, 0.96f), x, y, w, h);
		DrawText(label, Faint, x + 8.f, y + 4.f, Font, 0.82f);
		DrawText(val,   Cyan,  x + 8.f, y + 20.f, Font, 0.95f);
		AddHitBox(FVector2D(x, y), FVector2D(w, h), id, true);
	};

	auto DrawWrapped = [&](const FString& Text, const FLinearColor& Col, float x, float y, float maxW, float scale) -> float {
		TArray<FString> Words; Text.ParseIntoArray(Words, TEXT(" "));
		const float charW = 8.6f * scale;
		const int32 maxChars = FMath::Max(12, (int32)(maxW / charW));
		FString Ln; float ly = y;
		for (const FString& Wd : Words){
			if (Ln.Len() + Wd.Len() + 1 > maxChars){ DrawText(Ln, Col, x, ly, Font, scale); ly += 20.f * scale; Ln = Wd; }
			else { Ln = Ln.IsEmpty() ? Wd : Ln + TEXT(" ") + Wd; }
		}
		if (!Ln.IsEmpty()){ DrawText(Ln, Col, x, ly, Font, scale); ly += 20.f * scale; }
		return ly;
	};

	// name helpers
	auto CropName = [](ECropType C) -> const TCHAR* { switch (C){ case ECropType::Potato: return TEXT("Potato"); case ECropType::Lettuce: return TEXT("Lettuce"); case ECropType::Algae: return TEXT("Algae"); default: return TEXT("Legumes"); } };
	auto LightName    = [](ELightSource L) -> const TCHAR* { switch (L){ case ELightSource::LED: return TEXT("LED"); case ELightSource::Sun: return TEXT("Sun"); default: return TEXT("Hybrid"); } };
	auto WaterName    = [](EWaterSource V) -> const TCHAR* { return V == EWaterSource::IceMining ? TEXT("Ice/ISRU") : TEXT("Recycle"); };
	auto HeatName     = [](EHeatMode H)    -> const TCHAR* { return H == EHeatMode::Full ? TEXT("Full") : TEXT("Eco"); };
	auto NutrientName = [](ENutrientSource N) -> const TCHAR* { switch (N){ case ENutrientSource::Compost: return TEXT("Compost"); case ENutrientSource::Bacteria: return TEXT("Bacteria"); default: return TEXT("Earth"); } };
	auto StateName    = [](EGameState G)   -> const TCHAR* { switch (G){ case EGameState::Planning: return TEXT("PLANNING"); case EGameState::Running: return TEXT("RUNNING"); case EGameState::Event: return TEXT("DECISION"); case EGameState::Won: return TEXT("WON"); default: return TEXT("LOST"); } };

	const bool bEnded  = (S->State == EGameState::Won || S->State == EGameState::Lost);
	const bool bLive   = !bEnded && S->State != EGameState::Event;

	// dust screen tint
	if (S->DustLevel > 0.f)
		DrawRect(FLinearColor(0.55f, 0.26f, 0.09f, S->DustLevel * 0.34f), 0.f, 0.f, SW, SH);

	// ======================= CAMERA MONITOR (top-right, click to switch) =======================
	if (PC && PC->NumCameras() > 0)
	{
		const int32 NC = PC->NumCameras();
		const float tw = 178.f, th = 100.f, gap = 10.f, pad = 24.f;
		const float totalW = NC * tw + (NC - 1) * gap;
		const float x0 = SW - totalW - pad;
		const float y0 = 46.f;
		DrawText(TEXT("SECURITY CAMERAS  -  click a feed to switch room"), Faint, x0, y0 - 22.f, Font, 0.85f);
		for (int32 i = 0; i < NC; ++i)
		{
			AGreenhouseCamera* Cam = PC->GetCameraAt(i);
			const float x = x0 + i * (tw + gap);
			const bool bActive = (i == PC->CurrentCameraIndex());
			if (bActive) DrawRect(Orange, x - 3.f, y0 - 3.f, tw + 6.f, th + 6.f);
			DrawRect(FLinearColor(0.02f, 0.03f, 0.06f, 1.f), x, y0, tw, th);
			if (Cam && Cam->GetPreviewRT())
				DrawTexture(Cam->GetPreviewRT(), x, y0, tw, th, 0.f, 0.f, 1.f, 1.f);
			DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.62f), x, y0 + th - 20.f, tw, 20.f);
			DrawText(Cam ? FString::Printf(TEXT("%s / %s"), *Cam->RoomLabel, *Cam->CameraLabel) : FString::Printf(TEXT("CAM %d"), i),
				(bActive ? Amber : White), x + 6.f, y0 + th - 19.f, Font, 0.9f);
			AddHitBox(FVector2D(x, y0), FVector2D(tw, th), FName(*FString::Printf(TEXT("cam%d"), i)), true);
		}
	}

	// ======================= LEFT DASHBOARD =======================
	if (!bCustomDash)
	{
		const float PX = 28.f, PPAD = 16.f, PW = 452.f;
		float Y = 28.f;
		DrawRect(Panel, PX - PPAD, Y - PPAD, PW, 560.f);
		DrawText(TEXT("MARS GREENHOUSE"), Orange, PX, Y, Font, 1.4f); Y += 34.f;
		DrawText(FString::Printf(TEXT("SOL %d / %d"), S->Sol, Cfg->TotalSols), Grey, PX, Y, Font);
		DrawText(StateName(S->State), (S->State == EGameState::Running ? Green : Amber), PX + 150.f, Y, Font);
		DrawText(FString::Printf(TEXT("CAM %s"), PC ? *PC->GetCameraLabel() : TEXT("-")), Faint, PX + 268.f, Y, Font, 0.85f);
		Y += 28.f;
		Divider(PX, Y, PW - 2*PPAD); Y += 12.f;

		auto Meter = [&](const TCHAR* Name, float Val, const FLinearColor& Col)
		{
			DrawText(Name, Grey, PX, Y, Font);
			const float bx = PX + 118.f, bw = 232.f, bh = 16.f;
			DrawRect(FLinearColor(0.10f, 0.15f, 0.26f, 1.f), bx, Y + 2.f, bw, bh);
			const FLinearColor Fill = (Val <= 15.f) ? Red : Col;   // danger flash below 15%
			DrawRect(Fill, bx, Y + 2.f, bw * FMath::Clamp(Val * 0.01f, 0.f, 1.f), bh);
			DrawText(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Val)), White, bx + bw + 12.f, Y, Font);
			Y += 30.f;
		};
		Meter(TEXT("Oxygen"),   S->Oxygen,   Cyan);
		Meter(TEXT("Water"),    S->Water,    FLinearColor(0.20f, 0.72f, 0.80f));
		Meter(TEXT("Food"),     S->Food,     Green);
		Meter(TEXT("Nitrogen"), S->Nitrogen, Amber);
		Meter(TEXT("Power"),    S->Power,    Orange);
		Y += 4.f;
		Divider(PX, Y, PW - 2*PPAD); Y += 12.f;

		// sourcing chips (click to cycle)
		DrawText(TEXT("SOURCING  (click to change)"), Faint, PX, Y, Font, 0.82f); Y += 20.f;
		{
			const float cw = (PW - 2*PPAD - 3*8.f) / 4.f;
			Chip(PX + 0*(cw+8.f), Y, cw, TEXT("LIGHT"),    LightName(S->LightSource),    FName("src_light"));
			Chip(PX + 1*(cw+8.f), Y, cw, TEXT("WATER"),    WaterName(S->WaterSource),    FName("src_water"));
			Chip(PX + 2*(cw+8.f), Y, cw, TEXT("HEAT"),     HeatName(S->HeatMode),        FName("src_heat"));
			Chip(PX + 3*(cw+8.f), Y, cw, TEXT("NUTRIENT"), NutrientName(S->NutrientSource), FName("src_nutrient"));
			Y += 48.f;
		}
		DrawText(FString::Printf(TEXT("Dust %d%%     Earth-N left %d     Actions %d/%d"),
			FMath::RoundToInt(S->DustLevel * 100.f), S->EarthCreditsLeft, S->ActionsThisSol, Cfg->MaxActionsPerSol),
			(S->DustLevel > 0.f ? Amber : Faint), PX, Y, Font, 0.9f);
		Y += 24.f;
		Divider(PX, Y, PW - 2*PPAD); Y += 12.f;

		// beds (click a row to select)
		DrawText(TEXT("BEDS  (click to select)"), Cyan, PX, Y, Font, 0.9f); Y += 22.f;
		const int32 Sel = PC ? PC->SelectedBed : 0;
		const float bedsTop = Y;
		for (int32 i = 0; i < S->Beds.Num(); ++i)
		{
			const FPlantedBed& B = S->Beds[i];
			FString St;
			if (!B.bOccupied) St = B.bSoilClean ? TEXT("clean, empty - plant it") : TEXT("raw soil - WASH first");
			else St = FString::Printf(TEXT("%s   grow %d%%   hp %d%%"), CropName(B.Crop), FMath::RoundToInt(B.Growth * 100.f), FMath::RoundToInt(B.Health * 100.f));
			const bool bSel = (i == Sel);
			if (bSel) DrawRect(FLinearColor(0.20f, 0.14f, 0.04f, 0.9f), PX - 4.f, Y - 2.f, PW - 2*PPAD + 8.f, 24.f);
			DrawText(FString::Printf(TEXT("%s Bed %d   %s"), (bSel ? TEXT(">") : TEXT("  ")), i, *St), (bSel ? Amber : White), PX, Y, Font, 0.95f);
			AddHitBox(FVector2D(PX - 4.f, Y - 2.f), FVector2D(PW - 2*PPAD + 8.f, 24.f), FName(*FString::Printf(TEXT("bed%d"), i)), true);
			Y += 26.f;
		}
		SetHL(2, PX - 6.f, bedsTop - 4.f, PW - 2*PPAD + 12.f, (S->Beds.Num() * 26.f) + 6.f);

		// contextual plant buttons (only when the selected bed is clean + empty)
		Y += 6.f;
		const bool bCanPlant = S->Beds.IsValidIndex(Sel) && S->Beds[Sel].bSoilClean && !S->Beds[Sel].bOccupied;
		if (bCanPlant)
		{
			const float pw = (PW - 2*PPAD - 3*8.f) / 4.f, ph = 42.f;
			Button(PX + 0*(pw+8.f), Y, pw, ph, TEXT("Potato"),  TEXT("food"),  FLinearColor(0.16f,0.24f,0.14f,0.96f), FName("plant_potato"),  true);
			Button(PX + 1*(pw+8.f), Y, pw, ph, TEXT("Lettuce"), TEXT("fast"),  FLinearColor(0.16f,0.24f,0.14f,0.96f), FName("plant_lettuce"), true);
			Button(PX + 2*(pw+8.f), Y, pw, ph, TEXT("Algae"),   TEXT("O2"),    FLinearColor(0.16f,0.24f,0.14f,0.96f), FName("plant_algae"),   true);
			Button(PX + 3*(pw+8.f), Y, pw, ph, TEXT("Legumes"), TEXT("N-fix"), FLinearColor(0.16f,0.24f,0.14f,0.96f), FName("plant_legume"),  true);
			SetHL(3, PX - 4.f, Y - 4.f, PW - 2*PPAD + 8.f, ph + 8.f);
		}
		else
		{
			DrawText(TEXT("Select a clean, empty bed to show planting options."), Faint, PX, Y, Font, 0.85f);
		}
	}

	// ======================= BOTTOM ACTION BAR =======================
	{
		const float barH = 100.f;
		const float barY = SH - barH - 12.f;
		DrawRect(Panel, 12.f, barY - 8.f, SW - 24.f, barH);

		// row A: time controls
		float bx = 24.f; const float rowAY = barY;
		Button(bx, rowAY, 170.f, 42.f, TEXT("ADVANCE DAY"), TEXT("[Enter]"), FLinearColor(0.13f,0.30f,0.20f,0.98f), FName("advance"), bLive);
		SetHL(5, bx - 3.f, rowAY - 3.f, 176.f, 48.f);
		bx += 180.f;
		Button(bx, rowAY, 150.f, 42.f, (S->State == EGameState::Running ? TEXT("PAUSE / PLAN") : TEXT("RUN TIME")), TEXT("[Space]"), FLinearColor(0.12f,0.20f,0.32f,0.98f), FName("pause"), bLive);
		bx += 160.f;
		Button(bx, rowAY, 120.f, 42.f, TEXT("RESTART"), TEXT("[R]"), FLinearColor(0.28f,0.13f,0.13f,0.98f), FName("restart"), true);

		// row B: acquisition / maintenance actions
		float cx = 24.f; const float rowBY = barY + 50.f;
		const int32 Sel = PC ? PC->SelectedBed : 0;
		const bool bWash = S->Beds.IsValidIndex(Sel) && !S->Beds[Sel].bSoilClean && S->Water >= Cfg->SoilWashWaterCost;
		const bool bIce  = S->CanAct() && S->Power >= Cfg->MineIcePowerCost;
		const bool bEle  = S->CanAct() && S->Water >= Cfg->ElectrolyzeWaterCost && S->Power >= Cfg->ElectrolyzePowerCost;
		const bool bN2   = S->CanAct() && S->Power >= Cfg->HarvestN2PowerCost;
		const bool bOrd  = S->EarthCreditsLeft > 0;
		const FLinearColor ActFill(0.10f, 0.16f, 0.26f, 0.98f);

		Button(cx, rowBY, 138.f, 42.f, TEXT("WASH SOIL"),
			FString::Printf(TEXT("-%d H2O"), FMath::RoundToInt(Cfg->SoilWashWaterCost)), ActFill, FName("act_wash"), bWash); cx += 148.f;
		Button(cx, rowBY, 176.f, 42.f, TEXT("MINE ICE"),
			FString::Printf(TEXT("+%d H2O / -%d Pwr"), FMath::RoundToInt(Cfg->MineIceWater), FMath::RoundToInt(Cfg->MineIcePowerCost)), ActFill, FName("act_ice"), bIce); cx += 186.f;
		Button(cx, rowBY, 236.f, 42.f, TEXT("ELECTROLYZE"),
			FString::Printf(TEXT("+%d O2 / -%d H2O -%d Pwr"), FMath::RoundToInt(Cfg->ElectrolyzeOxygen), FMath::RoundToInt(Cfg->ElectrolyzeWaterCost), FMath::RoundToInt(Cfg->ElectrolyzePowerCost)), ActFill, FName("act_electro"), bEle); cx += 246.f;
		Button(cx, rowBY, 178.f, 42.f, TEXT("HARVEST N2"),
			FString::Printf(TEXT("+%d N / -%d Pwr"), FMath::RoundToInt(Cfg->HarvestN2Nitrogen), FMath::RoundToInt(Cfg->HarvestN2PowerCost)), ActFill, FName("act_n2"), bN2); cx += 188.f;
		Button(cx, rowBY, 150.f, 42.f, TEXT("ORDER N"),
			FString::Printf(TEXT("+%d N  (x%d)"), FMath::RoundToInt(Cfg->EarthOrderNitrogen), S->EarthCreditsLeft), ActFill, FName("act_orderN"), bOrd);

		SetHL(4, 20.f, rowBY - 4.f, cx + 150.f - 12.f, 50.f);
	}

	// ======================= EVENT CARD (clickable choices) =======================
	if (S->bEventActive && !bCustomEvent)
	{
		const float w = 760.f, h = 300.f;
		const float ex = SW * 0.5f - w * 0.5f, ey = SH * 0.42f - h * 0.5f;
		DrawRect(FLinearColor(0.05f, 0.07f, 0.14f, 0.97f), ex, ey, w, h);
		DrawRect(Orange, ex, ey, w, 5.f);
		DrawText(TEXT("INCOMING CALLOUT"), Amber, ex + 28.f, ey + 18.f, Font);
		DrawText(S->CurrentEvent.Speaker.ToString(), Cyan, ex + 28.f, ey + 42.f, Font, 1.2f);
		DrawWrapped(S->CurrentEvent.Situation.ToString(), White, ex + 28.f, ey + 78.f, w - 56.f, 1.0f);
		Button(ex + 28.f, ey + 150.f, w - 56.f, 44.f,
			FString::Printf(TEXT("A:  %s"), *S->CurrentEvent.ChoiceA.Label.ToString()), EffectArrows(S->CurrentEvent.ChoiceA.Effect),
			FLinearColor(0.12f,0.26f,0.16f,0.98f), FName("evt_a"), true);
		Button(ex + 28.f, ey + 204.f, w - 56.f, 44.f,
			FString::Printf(TEXT("B:  %s"), *S->CurrentEvent.ChoiceB.Label.ToString()), EffectArrows(S->CurrentEvent.ChoiceB.Effect),
			FLinearColor(0.26f,0.20f,0.10f,0.98f), FName("evt_b"), true);
	}

	// ======================= WIN / LOSE =======================
	if (bEnded)
	{
		const bool bWon = S->State == EGameState::Won;
		DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.6f), 0.f, SH * 0.5f - 90.f, SW, 200.f);
		DrawText(bWon ? TEXT("COLONY SURVIVED  -  YOU WIN") : TEXT("COLONY LOST"), bWon ? Green : Red, SW * 0.5f - 230.f, SH * 0.5f - 50.f, Font, 2.0f);
		Button(SW * 0.5f - 90.f, SH * 0.5f + 18.f, 180.f, 42.f, TEXT("RESTART RUN"), FString(), FLinearColor(0.20f,0.28f,0.40f,0.98f), FName("restart"), true);
	}

	// ======================= TUTORIAL BANNER =======================
	if (PC && PC->TutorialActive())
	{
		const float bw = 780.f, bx = SW * 0.5f - bw * 0.5f, by = 12.f, bh = 82.f;
		const float pulse = 0.5f + 0.5f * FMath::Sin(Now * 3.f);
		DrawRect(FLinearColor(0.04f, 0.06f, 0.13f, 0.95f), bx, by, bw, bh);
		DrawRect(FLinearColor(1.f, 0.55f, 0.20f, 0.35f + 0.45f * pulse), bx, by, bw, 3.f);
		DrawText(FString::Printf(TEXT("TUTORIAL  %d/6"), TStep + 1), Amber, bx + 16.f, by + 8.f, Font, 0.9f);
		DrawWrapped(PC->TutorialPrompt(), White, bx + 16.f, by + 30.f, bw - 210.f, 0.95f);
		Button(bx + bw - 96.f, by + bh - 34.f, 84.f, 26.f, TEXT("SKIP"), FString(), FLinearColor(0.30f,0.13f,0.13f,0.97f), FName("tut_skip"), true);
		if (PC->TutorialWantsNext())
			Button(bx + bw - 192.f, by + bh - 34.f, 90.f, 26.f, TEXT("NEXT"), FString(), FLinearColor(0.14f,0.30f,0.18f,0.97f), FName("tut_next"), true);

		// pulsing highlight around the element the player should use next
		if (bHL)
		{
			const float a = 0.45f + 0.45f * pulse;
			Outline(hlx, hly, hlw, hlh, FLinearColor(1.f, 0.72f, 0.25f, a), 3.f);
		}
	}
}
