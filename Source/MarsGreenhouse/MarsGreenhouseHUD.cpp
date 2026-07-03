// Copyright Epic Games, Inc. All Rights Reserved.
// Modern canvas HUD: color-coded resource cards, a grow-light segmented control, a selected-
// planter stage track, camera feeds + sol briefing, a bottom action dock, plus WORLD-ANCHORED
// action widgets and highlights drawn over the 3D planters/LED (projected to screen).

#include "MarsGreenhouseHUD.h"
#include "GreenhouseSimSubsystem.h"
#include "MarsSimSettings.h"
#include "MarsGreenhousePlayerController.h"
#include "GreenhouseCamera.h"
#include "GreenhouseBed.h"
#include "GreenhouseLED.h"
#include "Kismet/GameplayStatics.h"
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

	const float SW = Canvas->SizeX, SH = Canvas->SizeY;
	const float dt = W->GetDeltaSeconds();
	const float Now = W->GetTimeSeconds();
	const float pulse = 0.5f + 0.5f * FMath::Sin(Now * 3.5f);

	// type scale
	const float CAP = 0.82f, SUB = 0.92f, LBL = 1.14f, VAL = 1.55f, BIG = 2.1f;

	// palette (modern dark)
	const FLinearColor Prim  (0.92f, 0.95f, 0.99f);
	const FLinearColor Sec   (0.60f, 0.66f, 0.76f);
	const FLinearColor Muted (0.40f, 0.45f, 0.54f);
	const FLinearColor O2C   (0.27f, 0.84f, 0.77f);
	const FLinearColor H2OC  (0.35f, 0.63f, 0.93f);
	const FLinearColor FoodC (0.47f, 0.82f, 0.55f);
	const FLinearColor PwrC  (0.95f, 0.65f, 0.35f);
	const FLinearColor Accent(0.95f, 0.65f, 0.35f);
	const FLinearColor AccTxt(0.16f, 0.10f, 0.03f);
	const FLinearColor Red   (0.91f, 0.39f, 0.36f);
	const FLinearColor Green (0.47f, 0.82f, 0.55f);
	const FLinearColor Amber (0.95f, 0.72f, 0.34f);
	const FLinearColor RedLed(0.91f, 0.39f, 0.36f);
	const FLinearColor BluLed(0.35f, 0.55f, 1.00f);
	const FLinearColor CardBg(0.035f, 0.045f, 0.075f, 0.985f);
	const FLinearColor CardHi(0.06f, 0.08f, 0.12f, 0.99f);
	const FLinearColor Border(1.f, 1.f, 1.f, 0.16f);
	const FLinearColor BarBg (1.f, 1.f, 1.f, 0.09f);

	float mx = -1.f, my = -1.f;
	if (PC) PC->GetMousePosition(mx, my);

	auto MouseIn  = [&](float x, float y, float w, float h){ return mx>=x && mx<=x+w && my>=y && my<=y+h; };
	auto Brighten = [&](const FLinearColor& c){ return FLinearColor(FMath::Min(1.f,c.R*1.35f), FMath::Min(1.f,c.G*1.35f), FMath::Min(1.f,c.B*1.35f), c.A); };
	auto Outline  = [&](float x, float y, float w, float h, const FLinearColor& c, float t){ DrawRect(c,x,y,w,t); DrawRect(c,x,y+h-t,w,t); DrawRect(c,x,y,t,h); DrawRect(c,x+w-t,y,t,h); };
	auto Card     = [&](float x, float y, float w, float h, const FLinearColor& bg){ DrawRect(bg,x,y,w,h); Outline(x,y,w,h,Border,1.f); };
	auto Bar      = [&](float x, float y, float w, float h, float pct, const FLinearColor& col){ DrawRect(BarBg,x,y,w,h); DrawRect(col,x,y,w*FMath::Clamp(pct,0.f,1.f),h); };

	FName FrameHover; FString FrameTip;
	auto Button = [&](float x, float y, float w, float h, const FString& label, const FString& sub, const FLinearColor& fill, const FLinearColor& txt, FName id, bool enabled, const FString& tip)
	{
		const bool hov = enabled && MouseIn(x, y, w, h);
		const FLinearColor bg = !enabled ? FLinearColor(0.10f,0.12f,0.16f,0.85f) : (hov ? Brighten(fill) : fill);
		DrawRect(bg, x, y, w, h);
		DrawRect(FLinearColor(1,1,1, hov ? 0.16f : 0.08f), x, y, w, 2.f);
		DrawRect(FLinearColor(0,0,0,0.25f), x, y + h - 2.f, w, 2.f);
		Outline(x, y, w, h, hov ? Accent : Border, hov ? 2.f : 1.f);
		DrawText(label, enabled ? txt : Muted, x + 11.f, y + (sub.IsEmpty() ? h*0.5f - 9.f : 6.f), Font, LBL);
		if (!sub.IsEmpty()) DrawText(sub, enabled ? FLinearColor(txt.R*0.8f,txt.G*0.8f,txt.B*0.8f,0.9f) : Muted, x + 11.f, y + h - 17.f, Font, CAP);
		if (enabled) { AddHitBox(FVector2D(x,y), FVector2D(w,h), id, true); if (hov) { FrameHover = id; FrameTip = tip; } }
	};

	auto DrawWrapped = [&](const FString& Text, const FLinearColor& Col, float x, float y, float maxW, float scale) -> float
	{
		TArray<FString> Words; Text.ParseIntoArray(Words, TEXT(" "));
		const float charW = 9.0f * scale;
		const int32 maxChars = FMath::Max(10, (int32)(maxW / charW));
		FString Ln; float ly = y;
		for (const FString& Wd : Words)
		{
			if (Ln.Len() + Wd.Len() + 1 > maxChars) { DrawText(Ln, Col, x, ly, Font, scale); ly += 19.f * scale; Ln = Wd; }
			else { Ln = Ln.IsEmpty() ? Wd : Ln + TEXT(" ") + Wd; }
		}
		if (!Ln.IsEmpty()) { DrawText(Ln, Col, x, ly, Font, scale); ly += 19.f * scale; }
		return ly;
	};

	auto CropName = [](ECropType C){ return C == ECropType::Potato ? TEXT("Potato") : TEXT("Lettuce"); };
	auto StateStr = [](EGameState G){ switch (G){ case EGameState::Planning: return TEXT("Planning"); case EGameState::Event: return TEXT("Decision"); case EGameState::Won: return TEXT("Won"); default: return TEXT("Lost"); } };

	const bool bEnded = (S->State == EGameState::Won || S->State == EGameState::Lost);
	const bool bLive  = (S->State == EGameState::Planning);
	const int32 Sel   = PC ? PC->SelectedBed : 0;
	const int32 TStep = (PC && PC->TutorialActive()) ? PC->TutorialStepIndex() : -1;

	float Meters[4] = { S->Oxygen, S->Water, S->Food, S->Power };
	if (!bInitVals) { for (int32 i=0;i<4;++i){ DispVal[i]=Meters[i]; LastVal[i]=Meters[i]; } bInitVals = true; }

	auto BedStateColor = [&](const FPlantedBed& B, int32 bi) -> FLinearColor
	{
		if (!B.bOccupied) return Muted;
		if (B.bSpoiled)   return Red;
		if (S->IsBedReady(bi)) return Green;
		return Amber;
	};

	// dust tint
	if (S->DustLevel > 0.f)
		DrawRect(FLinearColor(0.55f, 0.26f, 0.09f, S->DustLevel * 0.32f), 0.f, 0.f, SW, SH);

	// ============================================================
	// WORLD-ANCHORED LAYER: highlight the 3D planters + LED, and
	// float each object's available actions right on top of it.
	// ============================================================
	if (!bEnded && !bCustomDash)
	{
		TArray<AActor*> BedActors;
		UGameplayStatics::GetAllActorsOfClass(this, AGreenhouseBed::StaticClass(), BedActors);
		for (AActor* A : BedActors)
		{
			AGreenhouseBed* Bed = Cast<AGreenhouseBed>(A);
			if (!Bed || !S->Beds.IsValidIndex(Bed->BedIndex)) continue;
			const int32 bi = Bed->BedIndex;
			const FVector Scr = Project(Bed->GetActorLocation() + FVector(0,0,60.f));
			if (Scr.Z <= 0.f) continue;
			const float px = Scr.X, py = Scr.Y;
			const FPlantedBed& B = S->Beds[bi];
			const bool ready = S->IsBedReady(bi);
			const bool sel   = (bi == Sel);
			const bool hov   = FMath::Abs(px-mx) < 55.f && FMath::Abs(py-my) < 44.f;
			const FLinearColor hc = BedStateColor(B, bi);

			// highlight bracket around the planter
			const float bxs = 30.f, bys = 24.f;
			const float thick = (sel || hov) ? 2.5f : 1.f;
			const FLinearColor oc = sel ? Accent : (ready ? FLinearColor(hc.R,hc.G,hc.B, 0.5f+0.5f*pulse) : (hov ? Prim : FLinearColor(hc.R,hc.G,hc.B,0.5f)));
			Outline(px-bxs, py-bys, bxs*2, bys*2, oc, thick);
			// state dot + planter tag
			DrawRect(hc, px-4.f, py-bys-10.f, 8.f, 8.f);
			DrawText(FString::Printf(TEXT("Planter %d"), bi), sel ? Accent : Sec, px-bxs, py+bys+4.f, Font, CAP);
			// clicking the highlight also selects
			AddHitBox(FVector2D(px-bxs, py-bys), FVector2D(bxs*2, bys*2), FName(*FString::Printf(TEXT("bed%d"), bi)), true);

			// action menu floats on the SELECTED planter
			if (sel && bLive)
			{
				const float mw = 150.f, ax = px - mw*0.5f, ay = py - bys - 78.f;
				if (!B.bOccupied)
				{
					const float hw = (mw-6.f)*0.5f;
					Button(ax,          ay, hw, 30.f, TEXT("Potato"),  FString(), FLinearColor(0.13f,0.22f,0.14f,0.96f), Prim, FName("plant_potato"),  true, TEXT("Plant a potato - slow, high food, likes red light."));
					Button(ax+hw+6.f,   ay, hw, 30.f, TEXT("Lettuce"), FString(), FLinearColor(0.13f,0.22f,0.14f,0.96f), Prim, FName("plant_lettuce"), true, TEXT("Plant lettuce - fast, good oxygen, likes blue light."));
				}
				else if (B.bSpoiled)
				{
					Button(ax, ay, mw, 30.f, TEXT("Clear planter"), FString(), FLinearColor(0.30f,0.14f,0.14f,0.96f), Prim, FName("act_harvest"), true, TEXT("Remove the spoiled crop to free the planter."));
				}
				else if (ready)
				{
					Button(ax, ay, mw, 30.f, TEXT("Harvest now"), FString(), FLinearColor(0.14f,0.34f,0.18f,0.98f), Prim, FName("act_harvest"), true, TEXT("Collect the crop before it spoils."));
				}
				else
				{
					const float hw = (mw-6.f)*0.5f;
					Button(ax,        ay, hw, 30.f, TEXT("Water"),   FString(), FLinearColor(0.12f,0.20f,0.32f,0.96f), Prim, FName("act_water"),   true, TEXT("Restore health so it keeps growing."));
					Button(ax+hw+6.f, ay, hw, 30.f, TEXT("Harvest"), FString(), FLinearColor(0.12f,0.24f,0.16f,0.96f), Prim, FName("act_harvest"), true, TEXT("Harvest early for a partial yield."));
				}
			}
		}

		// LED: hover it in the world to change the grow-light color
		TArray<AActor*> LedActors;
		UGameplayStatics::GetAllActorsOfClass(this, AGreenhouseLED::StaticClass(), LedActors);
		for (AActor* A : LedActors)
		{
			const FVector Scr = Project(A->GetActorLocation());
			if (Scr.Z <= 0.f) continue;
			const float px = Scr.X, py = Scr.Y;
			const bool bNear = FMath::Abs(px-mx) < 90.f && FMath::Abs(py-my) < 70.f;
			Outline(px-24.f, py-14.f, 48.f, 28.f, FLinearColor(1,1,1, bNear ? 0.5f : 0.18f), bNear ? 2.f : 1.f);
			if (bNear && bLive)
			{
				const float bw = 66.f, ax = px - (bw*3+8.f)*0.5f, ay = py - 60.f;
				Button(ax,             ay, bw, 28.f, TEXT("Blue"),     FString(), BluLed * (S->LedColor==ELedColor::Blue?1.f:0.4f),     Prim, FName("led_blue"),     true, TEXT("More oxygen, slower growth."));
				Button(ax+bw+4.f,      ay, bw, 28.f, TEXT("Balanced"), FString(), FLinearColor(0.5f,0.45f,0.6f, S->LedColor==ELedColor::Balanced?1.f:0.4f), Prim, FName("led_balanced"), true, TEXT("Solid all-round."));
				Button(ax+2*(bw+4.f),  ay, bw, 28.f, TEXT("Red"),      FString(), RedLed * (S->LedColor==ELedColor::Red?1.f:0.4f),      Prim, FName("led_red"),      true, TEXT("Faster growth, less oxygen."));
				break;
			}
		}
	}

	if (!bCustomDash)
	{
		// ============================= TOP STATUS BAR =============================
		DrawText(TEXT("MARS GREENHOUSE"), Accent, 20.f, 16.f, Font, LBL);
		{
			const float pw = 190.f, px = SW*0.5f - pw*0.5f;
			Card(px, 12.f, pw, 30.f, CardBg);
			DrawText(TEXT("SOL"), Muted, px + 12.f, 19.f, Font, CAP);
			DrawText(FString::Printf(TEXT("%d"), S->Sol), Prim, px + 42.f, 16.f, Font, LBL);
			DrawText(FString::Printf(TEXT("/ %d"), Cfg->TotalSols), Muted, px + 66.f, 19.f, Font, CAP);
			DrawText(StateStr(S->State), bLive ? Green : Amber, px + 110.f, 18.f, Font, SUB);
		}

		// storm alert (below the sol pill)
		{
			const float sbw = 520.f, sx = SW*0.5f - sbw*0.5f, sy = 48.f;
			if (S->DustLevel > 0.f)
			{
				Card(sx, sy, sbw, 34.f, FLinearColor(0.20f,0.06f,0.03f,0.94f));
				DrawRect(FLinearColor(0.91f,0.39f,0.15f, 0.4f+0.5f*pulse), sx, sy, sbw, 3.f);
				DrawText(FString::Printf(TEXT("DUST STORM   solar -%d%% this sol - mine ice / electrolyze while you can"), FMath::RoundToInt(S->DustLevel*100.f)), FLinearColor(0.98f,0.72f,0.62f), sx + 14.f, sy + 9.f, Font, SUB);
			}
			else if (S->NextDust > 0.f)
			{
				Card(sx, sy, sbw, 30.f, FLinearColor(0.14f,0.10f,0.03f,0.9f));
				DrawText(FString::Printf(TEXT("Storm forecast next sol (solar -%d%%) - bank power tonight"), FMath::RoundToInt(S->NextDust*100.f)), Amber, sx + 14.f, sy + 7.f, Font, SUB);
			}
		}

		// ============================= LEFT: RESOURCE CARDS =============================
		{
			const float LX = 16.f, LW = 168.f;
			float y = 92.f;
			const TCHAR* Names[4] = { TEXT("Oxygen"), TEXT("Water"), TEXT("Food"), TEXT("Power") };
			const FLinearColor Cols[4] = { O2C, H2OC, FoodC, PwrC };
			for (int32 i = 0; i < 4; ++i)
			{
				const float Val = Meters[i];
				DispVal[i] = FMath::FInterpTo(DispVal[i], Val, dt, 8.f);
				if (FMath::Abs(Val - LastVal[i]) >= 1.f)
				{
					LastDelta[i] = Val - LastVal[i];
					Floaters.Add({ FString::Printf(TEXT("%+d"), FMath::RoundToInt(LastDelta[i])), LastDelta[i] > 0 ? Green : Red, LX + LW - 10.f, y + 20.f, 1.3f });
				}
				LastVal[i] = Val;

				const float h = 64.f;
				const bool low = Val <= 15.f;
				Card(LX, y, LW, h, low ? FLinearColor(0.16f,0.06f,0.06f,0.9f) : CardBg);
				DrawRect(Cols[i], LX + 12.f, y + 13.f, 10.f, 10.f);
				DrawText(Names[i], Sec, LX + 30.f, y + 11.f, Font, SUB);
				if (LastDelta[i] != 0.f) DrawText(FString::Printf(TEXT("%+d"), FMath::RoundToInt(LastDelta[i])), LastDelta[i] > 0 ? Green : Red, LX + LW - 40.f, y + 11.f, Font, CAP);
				DrawText(FString::Printf(TEXT("%d"), FMath::RoundToInt(DispVal[i])), low ? Red : Prim, LX + 12.f, y + 26.f, Font, VAL);
				Bar(LX + 12.f, y + h - 14.f, LW - 24.f, 5.f, DispVal[i] * 0.01f, low ? Red : Cols[i]);
				y += h + 8.f;
			}
		}

		// ============================= CENTER: LIGHT + PLANTER =============================
		{
			// grow-light segmented control
			const float gw = 300.f, gx = SW*0.5f - gw*0.5f, gy = 92.f, seg = (gw-6.f)/3.f;
			Card(gx, gy, gw, 34.f, CardBg);
			auto Seg = [&](int32 i, const TCHAR* lbl, ELedColor col, const FLinearColor& c, FName id, const TCHAR* tip)
			{
				const float x = gx + 3.f + i * seg;
				const bool cur = (S->LedColor == col);
				Button(x, gy + 3.f, seg - 2.f, 28.f, lbl, FString(), cur ? c : FLinearColor(c.R*0.28f,c.G*0.28f,c.B*0.28f,0.6f), cur ? AccTxt : Sec, id, true, tip);
			};
			Seg(0, TEXT("Blue"),     ELedColor::Blue,     BluLed, FName("led_blue"),     TEXT("Blue light: more oxygen, slower growth. Best for lettuce."));
			Seg(1, TEXT("Balanced"), ELedColor::Balanced, FLinearColor(0.62f,0.55f,0.72f), FName("led_balanced"), TEXT("Balanced light: solid all-round."));
			Seg(2, TEXT("Red"),      ELedColor::Red,      RedLed, FName("led_red"),      TEXT("Red light: faster growth, bigger potatoes, less oxygen."));
			DrawText(TEXT("Grow-light"), Muted, gx, gy + 38.f, Font, CAP);

			// planter selector chips
			const float py0 = gy + 58.f;
			const int32 NB = S->Beds.Num();
			const float cw = 54.f, cg = 8.f, totalW = NB*cw + (NB-1)*cg, cx0 = SW*0.5f - totalW*0.5f;
			for (int32 i = 0; i < NB; ++i)
			{
				const float x = cx0 + i*(cw+cg);
				const bool sel = (i == Sel);
				const FLinearColor sc = BedStateColor(S->Beds[i], i);
				Card(x, py0, cw, 34.f, sel ? CardHi : CardBg);
				if (sel) Outline(x, py0, cw, 34.f, Accent, 2.f);
				DrawText(FString::Printf(TEXT("%d"), i), sel ? Accent : Sec, x + 9.f, py0 + 8.f, Font, LBL);
				DrawRect(sc, x + cw - 16.f, py0 + 13.f, 8.f, 8.f);
				AddHitBox(FVector2D(x, py0), FVector2D(cw, 34.f), FName(*FString::Printf(TEXT("bed%d"), i)), true);
			}

			// selected-planter card with stage track
			if (S->Beds.IsValidIndex(Sel))
			{
				const FPlantedBed& B = S->Beds[Sel];
				const float pw = 300.f, pxc = SW*0.5f - pw*0.5f, pyc = py0 + 44.f, ph = 78.f;
				Card(pxc, pyc, pw, ph, CardBg);
				DrawText(FString::Printf(TEXT("Planter %d"), Sel), Prim, pxc + 14.f, pyc + 10.f, Font, LBL);
				if (!B.bOccupied)
				{
					DrawText(TEXT("Empty - plant a crop"), Muted, pxc + 14.f, pyc + 38.f, Font, SUB);
				}
				else
				{
					const FCropStats& C = Cfg->GetCrop(B.Crop);
					DrawText(CropName(B.Crop), Amber, pxc + 110.f, pyc + 11.f, Font, SUB);
					DrawText(FString::Printf(TEXT("hp %d%%"), FMath::RoundToInt(B.Health*100.f)), B.Health < 0.35f ? Red : Sec, pxc + pw - 76.f, pyc + 11.f, Font, SUB);
					// stage labels
					const int32 stg = FMath::Clamp(CropStageIndex(C.StageEnds, B.Growth), 0, FMath::Max(0, C.StageNames.Num()-1));
					const int32 NS = C.StageNames.Num();
					for (int32 s = 0; s < NS; ++s)
					{
						const float sx = pxc + 14.f + s * ((pw-28.f)/FMath::Max(1,NS));
						DrawText(C.StageNames[s].ToString(), (s==stg && !B.bSpoiled) ? Amber : Muted, sx, pyc + 36.f, Font, CAP);
					}
					const FLinearColor pc = B.bSpoiled ? Red : (S->IsBedReady(Sel) ? Green : Amber);
					Bar(pxc + 14.f, pyc + ph - 16.f, pw - 28.f, 6.f, B.Growth, pc);
					if (B.bSpoiled) DrawText(FString::Printf(TEXT("Spoiled (%s)"), *C.SpoilName.ToString()), Red, pxc + pw - 130.f, pyc + 10.f, Font, CAP);
					else if (S->IsBedReady(Sel)) DrawText(FString::Printf(TEXT("READY  %d sol left"), S->SolsLeftToHarvest(Sel)), Green, pxc + pw - 150.f, pyc + 10.f, Font, CAP);
				}
			}
		}

		// ============================= RIGHT: CAMERAS + BRIEFING =============================
		const float RW = 188.f, RX = SW - RW - 16.f;
		if (PC && PC->NumCameras() > 0)
		{
			const int32 NC = FMath::Min(PC->NumCameras(), 2);
			const float tw = (RW - (NC-1)*6.f) / NC, th = 54.f, y0 = 42.f;
			for (int32 i = 0; i < PC->NumCameras(); ++i)
			{
				if (i >= NC) break;
				AGreenhouseCamera* Cam = PC->GetCameraAt(i);
				const float x = RX + i*(tw+6.f);
				const bool active = (i == PC->CurrentCameraIndex());
				DrawRect(FLinearColor(0.02f,0.03f,0.06f,1.f), x, y0, tw, th);
				if (Cam && Cam->GetPreviewRT()) DrawTexture(Cam->GetPreviewRT(), x, y0, tw, th, 0.f,0.f,1.f,1.f);
				Outline(x, y0, tw, th, active ? Accent : Border, active ? 2.f : 1.f);
				DrawText(Cam ? Cam->RoomLabel : FString::Printf(TEXT("Cam %d"),i), active ? Amber : Prim, x + 5.f, y0 + th - 16.f, Font, CAP);
				AddHitBox(FVector2D(x,y0), FVector2D(tw,th), FName(*FString::Printf(TEXT("cam%d"),i)), true);
			}
		}
		{
			const float by = 108.f, bh = 150.f;
			Card(RX, by, RW, bh, CardBg);
			DrawRect(Accent, RX, by, RW, 3.f);
			DrawText(TEXT("SOL BRIEFING"), Amber, RX + 12.f, by + 10.f, Font, CAP);
			float ty = DrawWrapped(S->CurrentFunFact.ToString(), Prim, RX + 12.f, by + 32.f, RW - 24.f, CAP);
			ty = FMath::Max(ty, by + bh - 46.f);
			Outline(RX + 12.f, ty, RW - 24.f, 1.f, Border, 1.f); ty += 8.f;
			DrawText(TEXT("Dust today"), Sec, RX + 12.f, ty, Font, CAP);
			DrawText(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(S->DustLevel*100.f)), S->DustLevel>0.f?Red:Sec, RX + RW - 46.f, ty, Font, CAP);
			DrawText(TEXT("Forecast"), Sec, RX + 12.f, ty + 18.f, Font, CAP);
			DrawText(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(S->NextDust*100.f)), S->NextDust>0.f?Amber:Sec, RX + RW - 46.f, ty + 18.f, Font, CAP);
		}
	}

	// ============================= BOTTOM ACTION DOCK =============================
	if (!bEnded && !bCustomDash)
	{
		const float M = 16.f, availW = SW - 2.f*M;
		const float barH = 60.f, barY = SH - barH - 12.f;
		Card(M - 4.f, barY - 8.f, availW + 8.f, barH + 4.f, CardBg);

		const bool bOcc   = S->Beds.IsValidIndex(Sel) && S->Beds[Sel].bOccupied;
		const bool bReady = S->IsBedReady(Sel);
		const bool bWater = bOcc && !S->Beds[Sel].bSpoiled && S->Water >= Cfg->WaterPlantWaterCost && S->Power >= Cfg->WaterPlantPowerCost && bLive;
		const bool bHarv  = bOcc && bLive;
		const bool bIce   = bLive && S->Power >= Cfg->MineIcePowerCost;
		const bool bEle   = bLive && S->Water >= Cfg->ElectrolyzeWaterCost && S->Power >= Cfg->ElectrolyzePowerCost;
		const FLinearColor AF(0.09f,0.13f,0.20f,0.96f);
		const FLinearColor HF = bReady ? FLinearColor(0.14f,0.34f,0.18f,0.98f) : AF;

		const float endW = 168.f;
		const int32 NB = 5;
		const float g = 8.f;
		const float dockW = availW - endW - g;
		const float bw = (dockW - (NB-1)*g) / NB, bh = 44.f, ry = barY + 6.f;
		auto AB = [&](int32 i, const FString& lbl, const FString& sub, const FLinearColor& c, FName id, bool en, const FString& tip)
		{ Button(M + i*(bw+g), ry, bw, bh, lbl, sub, c, Prim, id, en, tip); };
		AB(0, TEXT("Plant"),   TEXT("potato / lettuce"), AF, FName("plant_potato"),  bLive && !bOcc, TEXT("Plant a crop in the selected planter (or use the on-planter buttons)."));
		AB(1, TEXT("Water"),   FString::Printf(TEXT("-%d H2O"), FMath::RoundToInt(Cfg->WaterPlantWaterCost)), AF, FName("act_water"), bWater, TEXT("Water the selected planter. Neglected plants die."));
		AB(2, bReady ? TEXT("Harvest !") : TEXT("Harvest"), TEXT("[E]"), HF, FName("act_harvest"), bHarv, TEXT("Collect the crop. Early = partial; too late = spoiled."));
		AB(3, TEXT("Mine ice"), FString::Printf(TEXT("+%d -%d Pwr"), FMath::RoundToInt(Cfg->MineIceWater), FMath::RoundToInt(Cfg->MineIcePowerCost)), AF, FName("act_ice"), bIce, TEXT("Extract water from ice. Costs power."));
		AB(4, TEXT("Electrolyze"), FString::Printf(TEXT("+%d O2"), FMath::RoundToInt(Cfg->ElectrolyzeOxygen)), AF, FName("act_electro"), bEle, TEXT("Split water into oxygen. Costs water + power."));
		Button(SW - M - endW, ry, endW, bh, TEXT("END DAY"), TEXT("[Enter]"), Accent, AccTxt, FName("advance"), bLive, TEXT("Resolve the sol: crew eats, crops grow, weather turns."));
	}

	// ============================= EVENT / NARRATIVE CARD =============================
	if (S->bEventActive && !bCustomEvent)
	{
		const float w = 720.f, h = 292.f, ex = SW*0.5f - w*0.5f, ey = SH*0.40f - h*0.5f;
		Card(ex, ey, w, h, FLinearColor(0.05f,0.07f,0.13f,0.98f));
		DrawRect(Accent, ex, ey, w, 4.f);
		DrawText(S->CurrentEvent.bSingleChoice ? TEXT("STORY") : TEXT("DECISION"), Amber, ex + 26.f, ey + 16.f, Font, CAP);
		DrawText(S->CurrentEvent.Speaker.ToString(), O2C, ex + 26.f, ey + 38.f, Font, VAL);
		DrawWrapped(S->CurrentEvent.Situation.ToString(), Prim, ex + 26.f, ey + 76.f, w - 52.f, LBL);
		if (S->CurrentEvent.bSingleChoice)
		{
			Button(ex + 26.f, ey + h - 62.f, w - 52.f, 44.f, S->CurrentEvent.ChoiceA.Label.ToString(), FString(), FLinearColor(0.12f,0.26f,0.16f,0.98f), Prim, FName("evt_a"), true, FString());
		}
		else
		{
			Button(ex + 26.f, ey + h - 96.f, w - 52.f, 42.f, FString::Printf(TEXT("A:  %s"), *S->CurrentEvent.ChoiceA.Label.ToString()), EffectArrows(S->CurrentEvent.ChoiceA.Effect), FLinearColor(0.12f,0.26f,0.16f,0.98f), Prim, FName("evt_a"), true, FString());
			Button(ex + 26.f, ey + h - 48.f, w - 52.f, 38.f, FString::Printf(TEXT("B:  %s"), *S->CurrentEvent.ChoiceB.Label.ToString()), EffectArrows(S->CurrentEvent.ChoiceB.Effect), FLinearColor(0.26f,0.20f,0.10f,0.98f), Prim, FName("evt_b"), true, FString());
		}
	}

	// ============================= WIN / LOSE (scored) =============================
	if (bEnded)
	{
		const bool bWon = S->State == EGameState::Won;
		DrawRect(FLinearColor(0.02f,0.03f,0.05f,0.9f), 0.f, 0.f, SW, SH);
		const float cw = 420.f, cx = SW*0.5f - cw*0.5f, cy = SH*0.5f - 150.f;
		Card(cx, cy, cw, 300.f, CardHi);
		DrawRect(bWon ? Green : Red, cx, cy, cw, 4.f);
		DrawText(bWon ? TEXT("COLONY SURVIVED") : TEXT("COLONY LOST"), bWon ? Green : Red, cx + 30.f, cy + 26.f, Font, BIG);
		DrawText(FString::Printf(TEXT("Sols survived:  %d / %d"), FMath::Min(S->Sol - 1, Cfg->TotalSols), Cfg->TotalSols), Prim, cx + 30.f, cy + 92.f, Font, VAL);
		DrawText(FString::Printf(TEXT("Harvests:  %d"), S->Harvests), Sec, cx + 30.f, cy + 126.f, Font, LBL);
		DrawText(FString::Printf(TEXT("Crops lost:  %d"), S->Spoiled), Sec, cx + 220.f, cy + 126.f, Font, LBL);
		DrawText(TEXT("RANK"), Muted, cx + 30.f, cy + 168.f, Font, SUB);
		DrawText(S->ResultRank(), Amber, cx + 110.f, cy + 158.f, Font, BIG);
		Button(cx + 30.f, cy + 232.f, cw - 60.f, 44.f, TEXT("RESTART RUN"), FString(), Accent, AccTxt, FName("restart"), true, FString());
	}

	// ============================= TUTORIAL =============================
	if (PC && PC->TutorialActive() && !bEnded)
	{
		const float bw = 800.f, bx = SW*0.5f - bw*0.5f, by = SH - 172.f, bh = 66.f;
		Card(bx, by, bw, bh, FLinearColor(0.05f,0.07f,0.13f,0.96f));
		DrawRect(FLinearColor(1.f,0.55f,0.20f, 0.35f+0.45f*pulse), bx, by, bw, 3.f);
		DrawText(FString::Printf(TEXT("TUTORIAL  %d/6"), TStep + 1), Amber, bx + 16.f, by + 8.f, Font, CAP);
		DrawWrapped(PC->TutorialPrompt(), Prim, bx + 16.f, by + 26.f, bw - 210.f, SUB);
		Button(bx + bw - 92.f, by + bh - 30.f, 82.f, 24.f, TEXT("Skip"), FString(), FLinearColor(0.30f,0.13f,0.13f,0.97f), Prim, FName("tut_skip"), true, FString());
		if (PC->TutorialWantsNext())
			Button(bx + bw - 184.f, by + bh - 30.f, 86.f, 24.f, TEXT("Next"), FString(), FLinearColor(0.14f,0.30f,0.18f,0.97f), Prim, FName("tut_next"), true, FString());
	}

	// ============================= FLOATERS =============================
	for (int32 i = Floaters.Num() - 1; i >= 0; --i)
	{
		FFloatText& F = Floaters[i];
		F.Life -= dt; F.Y -= 30.f * dt;
		if (F.Life <= 0.f) { Floaters.RemoveAt(i); continue; }
		FLinearColor C = F.Color; C.A = FMath::Clamp(F.Life / 1.3f, 0.f, 1.f);
		DrawText(F.Text, C, F.X, F.Y, Font, VAL);
	}

	// ============================= TOOLTIP =============================
	if (FrameHover == HoverBox) HoverTime += dt; else { HoverBox = FrameHover; HoverTime = 0.f; }
	if (HoverTime > 0.6f && !FrameTip.IsEmpty() && mx > 0.f)
	{
		const float tw = 280.f, tx = FMath::Min(mx + 16.f, SW - tw - 8.f), ty = my + 18.f;
		Card(tx, ty, tw, 48.f, FLinearColor(0.02f,0.03f,0.07f,0.97f));
		DrawRect(Accent, tx, ty, tw, 2.f);
		DrawWrapped(FrameTip, Prim, tx + 8.f, ty + 8.f, tw - 16.f, CAP);
	}
}
