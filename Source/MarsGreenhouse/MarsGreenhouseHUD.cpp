// Copyright Epic Games, Inc. All Rights Reserved.
// "Bustan" HUD — reference-inspired: top segmented resource bar, crew status, mission log,
// objective, world-anchored labels, crop cards, warm Mars palette. Every art slot has a
// code-drawn fallback (rounded cards, primitive icons) so it works before art arrives.

#include "MarsGreenhouseHUD.h"
#include "GreenhouseSimSubsystem.h"
#include "MarsSimSettings.h"
#include "MarsGreenhousePlayerController.h"
#include "GreenhouseCamera.h"
#include "GreenhouseBed.h"
#include "GreenhouseLED.h"
#include "GreenhouseLabel.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
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
	auto L = [&](const TSoftObjectPtr<UTexture2D>& P) -> UTexture2D* { return P.IsNull() ? nullptr : P.LoadSynchronous(); };
	UFont* CustomFont = Cfg->HudFont.IsNull() ? nullptr : Cfg->HudFont.LoadSynchronous();
	UFont* Font = CustomFont ? CustomFont : (GEngine ? GEngine->GetMediumFont() : nullptr);
	UTexture2D* ResTex[4]  = { L(Cfg->IconOxygen), L(Cfg->IconWater), L(Cfg->IconFood), L(Cfg->IconPower) };
	UTexture2D* PanelTex   = L(Cfg->PanelFrame);
	UTexture2D* ButtonTex  = L(Cfg->ButtonFrame);
	UTexture2D* VignetteTex = L(Cfg->VignetteOverlay);
	UTexture2D* LogoTex    = L(Cfg->TitleLogo);

	AMarsGreenhousePlayerController* PC = Cast<AMarsGreenhousePlayerController>(PlayerOwner);
	const bool bCustomDash  = PC && PC->HasCustomDashboard();
	const bool bCustomEvent = PC && PC->HasCustomEventUI();

	const float SW = Canvas->SizeX, SH = Canvas->SizeY;
	const float dt = W->GetDeltaSeconds();
	const float Now = W->GetTimeSeconds();
	const float pulse = 0.5f + 0.5f * FMath::Sin(Now * 3.5f);
	const float CAP = 0.82f, SUB = 0.92f, LBL = 1.14f, VAL = 1.5f, BIG = 2.0f;

	// modern cool-graphite palette (cyan accent)
	const FLinearColor Prim  (0.93f, 0.95f, 0.98f);
	const FLinearColor Sec   (0.60f, 0.66f, 0.75f);
	const FLinearColor Muted (0.42f, 0.47f, 0.56f);
	const FLinearColor O2C   (0.32f, 0.83f, 0.80f);
	const FLinearColor H2OC  (0.38f, 0.63f, 0.96f);
	const FLinearColor FoodC (0.50f, 0.82f, 0.52f);
	const FLinearColor PwrC  (0.97f, 0.68f, 0.34f);
	const FLinearColor Accent(0.28f, 0.80f, 0.88f);   // modern cyan
	const FLinearColor AccTxt(0.02f, 0.10f, 0.12f);
	const FLinearColor Red   (0.95f, 0.44f, 0.44f);
	const FLinearColor Green (0.44f, 0.85f, 0.56f);
	const FLinearColor Amber (0.45f, 0.80f, 0.92f);   // cool label/accent (headers, growing state)
	const FLinearColor PurpleC(0.64f, 0.46f, 0.98f);
	const FLinearColor WhiteC (0.96f, 0.97f, 1.00f);
	const FLinearColor CardBg(0.055f, 0.065f, 0.090f, 0.972f);   // cool slate
	const FLinearColor CardHi(0.095f, 0.11f, 0.145f, 0.982f);
	const FLinearColor IconBg(0.06f, 0.07f, 0.095f, 1.f);
	const FLinearColor Border(0.55f, 0.68f, 0.92f, 0.15f);
	const FLinearColor BarBg (1.f, 1.f, 1.f, 0.08f);

	float mx = -1.f, my = -1.f;
	if (PC) PC->GetMousePosition(mx, my);

	auto MouseIn  = [&](float x, float y, float w, float h){ return mx>=x && mx<=x+w && my>=y && my<=y+h; };
	auto Brighten = [&](const FLinearColor& c){ return FLinearColor(FMath::Min(1.f,c.R*1.3f), FMath::Min(1.f,c.G*1.3f), FMath::Min(1.f,c.B*1.3f), c.A); };
	auto RoundRect = [&](float x, float y, float w, float h, float r, const FLinearColor& col)
	{
		r = FMath::Clamp(r, 0.f, FMath::Min(w, h) * 0.5f);
		if (r < 1.5f) { DrawRect(col, x, y, w, h); return; }
		DrawRect(col, x, y + r, w, h - 2.f*r);
		const int32 ri = FMath::CeilToInt(r);
		for (int32 i = 0; i < ri; ++i)
		{
			const float dyv = (float)i + 0.5f;
			const float dxv = r - FMath::Sqrt(FMath::Max(0.f, r*r - (r - dyv)*(r - dyv)));
			DrawRect(col, x + dxv, y + i, w - 2.f*dxv, 1.f);
			DrawRect(col, x + dxv, y + h - 1.f - i, w - 2.f*dxv, 1.f);
		}
	};
	auto Circle = [&](float cx, float cy, float rad, const FLinearColor& col){ RoundRect(cx-rad, cy-rad, rad*2.f, rad*2.f, rad, col); };
	auto Outline = [&](float x, float y, float w, float h, const FLinearColor& c, float t){ DrawRect(c,x,y,w,t); DrawRect(c,x,y+h-t,w,t); DrawRect(c,x,y,t,h); DrawRect(c,x+w-t,y,t,h); };
	// Fallback icons from circles / rounded-rects (no triangle item, so no GWhiteTexture dependency).
	auto Icon = [&](int32 type, float x, float y, float s, const FLinearColor& col)
	{
		const float cx = x + s*0.5f, cy = y + s*0.5f;
		switch (type)
		{
			case 0: Circle(cx, cy, s*0.5f, col); Circle(cx, cy, s*0.28f, IconBg); break;                                    // oxygen ring
			case 1: Circle(cx, cy + s*0.06f, s*0.42f, col); RoundRect(x+s*0.42f, y, s*0.16f, s*0.5f, s*0.08f, col); break;   // water drop
			case 2: RoundRect(x+s*0.12f, y+s*0.12f, s*0.76f, s*0.76f, s*0.24f, col); break;                                  // food square
			default: RoundRect(x+s*0.36f, y+s*0.05f, s*0.28f, s*0.9f, s*0.1f, col); break;                                   // power bar
		}
	};
	// draw a resource icon: texture if assigned, else the primitive shape
	auto ResIcon = [&](int32 i, float x, float y, float s, const FLinearColor& col){ if (ResTex[i]) DrawTexture(ResTex[i], x, y, s, s, 0.f,0.f,1.f,1.f, col); else Icon(i, x, y, s, col); };

	// 9-slice: stretch a frame texture to any size keeping corners crisp
	auto NineSlice = [&](UTexture2D* Tex, float x, float y, float w, float h, float margin, const FLinearColor& tint)
	{
		const float ts = Tex ? (float)Tex->GetSizeX() : 0.f;
		if (ts <= 0.f) return;
		const float m = FMath::Min(margin, FMath::Min(w, h) * 0.5f);
		const float um = margin / ts;
		const float DX[3] = { x, x + m, x + w - m };
		const float DW[3] = { m, w - 2.f*m, m };
		const float DY[3] = { y, y + m, y + h - m };
		const float DH[3] = { m, h - 2.f*m, m };
		const float U[3]  = { 0.f, um, 1.f - um };
		const float UL[3] = { um, 1.f - 2.f*um, um };
		for (int32 r = 0; r < 3; ++r)
			for (int32 c = 0; c < 3; ++c)
				DrawTexture(Tex, DX[c], DY[r], DW[c], DH[r], U[c], U[r], UL[c], UL[r], tint);
	};

	auto Card = [&](float x, float y, float w, float h, const FLinearColor& bg)
	{
		if (PanelTex) NineSlice(PanelTex, x, y, w, h, Cfg->PanelMargin, FLinearColor::White);
		else { RoundRect(x, y, w, h, 10.f, Border); RoundRect(x+1.f, y+1.f, w-2.f, h-2.f, 9.f, bg); }
	};
	auto Bar = [&](float x, float y, float w, float h, float pct, const FLinearColor& col){ RoundRect(x,y,w,h,h*0.5f,BarBg); const float fw=w*FMath::Clamp(pct,0.f,1.f); if (fw>h) RoundRect(x,y,fw,h,h*0.5f,col); };
	auto SegMeter = [&](float x, float y, float w, float h, float pct, const FLinearColor& col)
	{
		const int32 cells = 10; const float gap = 2.f;
		const float cw = (w - (cells-1)*gap) / cells;
		const int32 filled = FMath::RoundToInt(FMath::Clamp(pct,0.f,1.f) * cells);
		for (int32 i = 0; i < cells; ++i)
			RoundRect(x + i*(cw+gap), y, cw, h, 1.5f, i < filled ? col : BarBg);
	};

	FName FrameHover; FString FrameTip;
	auto Button = [&](float x, float y, float w, float h, const FString& label, const FString& sub, const FLinearColor& fill, const FLinearColor& txt, FName id, bool enabled, const FString& tip)
	{
		const bool hov = enabled && MouseIn(x, y, w, h);
		const FLinearColor bg = !enabled ? FLinearColor(0.12f,0.09f,0.07f,0.9f) : (hov ? Brighten(fill) : fill);
		if (ButtonTex) NineSlice(ButtonTex, x, y, w, h, Cfg->ButtonMargin, bg);
		else { if (hov) RoundRect(x-2.f,y-2.f,w+4.f,h+4.f,9.f,Accent); RoundRect(x, y, w, h, 7.f, bg); }
		DrawText(label, enabled ? txt : Muted, x + 12.f, y + (sub.IsEmpty() ? h*0.5f - 9.f : 6.f), Font, LBL);
		if (!sub.IsEmpty()) DrawText(sub, enabled ? FLinearColor(txt.R*0.82f,txt.G*0.82f,txt.B*0.82f,0.92f) : Muted, x + 12.f, y + h - 17.f, Font, CAP);
		if (enabled) { AddHitBox(FVector2D(x,y), FVector2D(w,h), id, true); if (hov) { FrameHover = id; FrameTip = tip; } }
	};
	auto DrawWrapped = [&](const FString& Text, const FLinearColor& Col, float x, float y, float maxW, float scale) -> float
	{
		TArray<FString> Words; Text.ParseIntoArray(Words, TEXT(" "));
		const float charW = 9.0f * scale; const int32 maxChars = FMath::Max(10, (int32)(maxW / charW));
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

	if (VignetteTex) DrawTexture(VignetteTex, 0.f, 0.f, SW, SH, 0.f,0.f,1.f,1.f, FLinearColor(1,1,1,0.5f));
	if (S->DustLevel > 0.f) DrawRect(FLinearColor(0.55f,0.26f,0.09f, S->DustLevel*0.30f), 0.f, 0.f, SW, SH);

	// ================= WORLD LABELS + planter highlights + on-planter menus =================
	if (!bEnded && !bCustomDash)
	{
		auto WorldTag = [&](const FVector& Loc, const FString& Txt, const FLinearColor& col, bool bold)
		{
			const FVector Scr = Project(Loc);
			if (Scr.Z <= 0.f) return FVector2D(-1.f,-1.f);
			const float tw = 10.f + Txt.Len() * 8.2f;
			const float tx = Scr.X - tw*0.5f, ty = Scr.Y;
			RoundRect(tx, ty, tw, 22.f, 6.f, FLinearColor(0.06f,0.045f,0.03f,0.9f));
			Outline(tx, ty, tw, 22.f, col, 1.f);
			DrawText(Txt, bold ? col : Prim, tx + 8.f, ty + 4.f, Font, CAP);
			return FVector2D(Scr.X, Scr.Y);
		};

		// placeable labels on props
		TArray<AActor*> LabelActors;
		UGameplayStatics::GetAllActorsOfClass(this, AGreenhouseLabel::StaticClass(), LabelActors);
		for (AActor* A : LabelActors)
			if (AGreenhouseLabel* Lab = Cast<AGreenhouseLabel>(A))
				WorldTag(Lab->GetActorLocation() + FVector(0,0,Lab->HeightOffset), Lab->LabelText, Amber, false);

		// planters: highlight + label + on-planter action menu
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
			const float bxs = 30.f, bys = 24.f;
			const FLinearColor oc = sel ? Accent : (ready ? FLinearColor(hc.R,hc.G,hc.B, 0.5f+0.5f*pulse) : (hov ? Prim : FLinearColor(hc.R,hc.G,hc.B,0.5f)));
			Outline(px-bxs, py-bys, bxs*2, bys*2, oc, (sel||hov)?2.5f:1.f);
			Circle(px, py-bys-6.f, 5.f, hc);
			DrawText(FString::Printf(TEXT("Planter %d"), bi), sel ? Accent : Sec, px-bxs, py+bys+4.f, Font, CAP);
			AddHitBox(FVector2D(px-bxs, py-bys), FVector2D(bxs*2, bys*2), FName(*FString::Printf(TEXT("bed%d"), bi)), true);

			if (sel && bLive)
			{
				const float mw = 150.f, ax = px - mw*0.5f, ay = py - bys - 78.f;
				if (!B.bOccupied)
				{
					const float hw = (mw-6.f)*0.5f;
					Button(ax,        ay, hw, 30.f, TEXT("Potato"),  FString(), FLinearColor(0.14f,0.20f,0.12f,0.98f), Prim, FName("plant_potato"),  true, TEXT("Plant potato - slow, high food, likes white light."));
					Button(ax+hw+6.f, ay, hw, 30.f, TEXT("Lettuce"), FString(), FLinearColor(0.14f,0.20f,0.12f,0.98f), Prim, FName("plant_lettuce"), true, TEXT("Plant lettuce - fast, good oxygen, likes purple light."));
				}
				else if (B.bSpoiled) Button(ax, ay, mw, 30.f, TEXT("Clear planter"), FString(), FLinearColor(0.30f,0.13f,0.10f,0.98f), Prim, FName("act_harvest"), true, TEXT("Remove the spoiled crop."));
				else if (ready)      Button(ax, ay, mw, 30.f, TEXT("Harvest now"),   FString(), FLinearColor(0.16f,0.30f,0.16f,0.99f), Prim, FName("act_harvest"), true, TEXT("Collect before it spoils."));
				else
				{
					const float hw = (mw-6.f)*0.5f;
					Button(ax,        ay, hw, 30.f, TEXT("Water"),   FString(), FLinearColor(0.14f,0.18f,0.30f,0.98f), Prim, FName("act_water"),   true, TEXT("Restore health."));
					Button(ax+hw+6.f, ay, hw, 30.f, TEXT("Harvest"), FString(), FLinearColor(0.14f,0.24f,0.14f,0.98f), Prim, FName("act_harvest"), true, TEXT("Harvest early (partial)."));
				}
			}
		}

		// LED world tag + hover switcher
		TArray<AActor*> LedActors;
		UGameplayStatics::GetAllActorsOfClass(this, AGreenhouseLED::StaticClass(), LedActors);
		for (AActor* A : LedActors)
		{
			const FVector2D P = WorldTag(A->GetActorLocation() + FVector(0,0,50.f), TEXT("Grow Lights"), S->LedColor==ELedColor::White?WhiteC:PurpleC, true);
			if (P.X < 0.f) continue;
			const bool bNear = FMath::Abs(P.X-mx) < 90.f && FMath::Abs(P.Y-my) < 70.f;
			if (bNear && bLive)
			{
				const float bw = 84.f, ax = P.X - (bw*2+6.f)*0.5f, ay = P.Y - 40.f;
				Button(ax,        ay, bw, 28.f, TEXT("Purple"), FString(), PurpleC * (S->LedColor==ELedColor::Purple?1.f:0.4f), Prim, FName("led_purple"), true, TEXT("More oxygen, steady growth."));
				Button(ax+bw+6.f, ay, bw, 28.f, TEXT("White"),  FString(), WhiteC  * (S->LedColor==ELedColor::White ?1.f:0.4f), AccTxt, FName("led_white"),  true, TEXT("Faster growth, less oxygen."));
				break;
			}
		}
	}

	if (!bCustomDash)
	{
		// ================= TOP RESOURCE BAR (segmented) =================
		{
			const TCHAR* RN[4] = { TEXT("Oxygen"), TEXT("Water"), TEXT("Food"), TEXT("Power") };
			const FLinearColor RC[4] = { O2C, H2OC, FoodC, PwrC };
			const float bw = 214.f, bh = 46.f; float bx = 14.f, by = 10.f;
			for (int32 i = 0; i < 4; ++i)
			{
				const float Val = Meters[i];
				DispVal[i] = FMath::FInterpTo(DispVal[i], Val, dt, 8.f);
				if (FMath::Abs(Val - LastVal[i]) >= 1.f) { LastDelta[i] = Val - LastVal[i]; Floaters.Add({ FString::Printf(TEXT("%+d"), FMath::RoundToInt(LastDelta[i])), LastDelta[i]>0?Green:Red, bx + bw*0.5f, by + bh + 4.f, 1.3f }); }
				LastVal[i] = Val;
				const bool low = Val <= 15.f;
				Card(bx, by, bw, bh, low ? FLinearColor(0.17f,0.06f,0.05f,0.97f) : CardBg);
				Circle(bx + 24.f, by + 23.f, 15.f, FLinearColor(RC[i].R,RC[i].G,RC[i].B,0.16f));
				ResIcon(i, bx + 13.f, by + 12.f, 22.f, RC[i]);
				DrawText(RN[i], Sec, bx + 46.f, by + 6.f, Font, CAP);
				DrawText(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(DispVal[i])), low ? Red : Prim, bx + bw - 46.f, by + 5.f, Font, SUB);
				SegMeter(bx + 46.f, by + 26.f, bw - 60.f, 10.f, DispVal[i]*0.01f, low ? Red : RC[i]);
				bx += bw + 8.f;
			}
			// SOL pill (far right)
			const float pw = 132.f, pxp = SW - pw - 14.f;
			Card(pxp, by, pw, bh, CardBg);
			DrawText(TEXT("SOL"), Muted, pxp + 14.f, by + 7.f, Font, CAP);
			DrawText(FString::Printf(TEXT("%02d"), S->Sol), Accent, pxp + 14.f, by + 20.f, Font, VAL);
			DrawText(FString::Printf(TEXT("/ %d"), Cfg->TotalSols), Sec, pxp + 58.f, by + 26.f, Font, CAP);
			DrawText(bLive ? TEXT("DAY") : TEXT("..."), Sec, pxp + pw - 44.f, by + 8.f, Font, CAP);
		}

		// storm banner
		{
			const float sbw = 500.f, sx = SW*0.5f - sbw*0.5f, sy = 64.f;
			if (S->DustLevel > 0.f) { Card(sx, sy, sbw, 32.f, FLinearColor(0.22f,0.07f,0.03f,0.96f)); DrawText(FString::Printf(TEXT("DUST STORM   solar -%d%% this sol"), FMath::RoundToInt(S->DustLevel*100.f)), FLinearColor(0.98f,0.72f,0.6f), sx + 16.f, sy + 8.f, Font, SUB); }
			else if (S->NextDust > 0.f) { Card(sx, sy, sbw, 28.f, FLinearColor(0.16f,0.11f,0.03f,0.94f)); DrawText(FString::Printf(TEXT("Storm forecast next sol (solar -%d%%) - bank power"), FMath::RoundToInt(S->NextDust*100.f)), Amber, sx + 16.f, sy + 6.f, Font, SUB); }
		}

		// ================= RIGHT: CREW STATUS =================
		{
			const float cw = 210.f, cx = SW - cw - 14.f, cyTop = 66.f;
			const int32 NCrew = Cfg->Crew.Num();
			const float rowH = 42.f, ph = 26.f + NCrew * rowH;
			Card(cx, cyTop, cw, ph, CardBg);
			DrawText(TEXT("CREW STATUS"), Amber, cx + 14.f, cyTop + 8.f, Font, CAP);
			for (int32 i = 0; i < NCrew; ++i)
			{
				const float ry = cyTop + 26.f + i * rowH;
				UTexture2D* Portrait = Cfg->CrewPortraits.IsValidIndex(i) ? L(Cfg->CrewPortraits[i]) : nullptr;
				if (Portrait) DrawTexture(Portrait, cx + 12.f, ry + 3.f, 30.f, 30.f, 0.f,0.f,1.f,1.f);
				else { Circle(cx + 27.f, ry + 18.f, 15.f, FLinearColor(0.2f,0.15f,0.11f,1.f)); DrawText(Cfg->Crew[i].Name.ToString().Left(1), Amber, cx + 22.f, ry + 9.f, Font, SUB); }
				DrawText(Cfg->Crew[i].Name.ToString(), Prim, cx + 50.f, ry + 3.f, Font, CAP);
				DrawText(Cfg->Crew[i].Role.ToString(), Muted, cx + 50.f, ry + 19.f, Font, CAP);
				const FString St = S->GetCrewStatus(i);
				const FLinearColor sc = (St == TEXT("OK")) ? Green : (St == TEXT("Critical") ? Red : Amber);
				DrawText(St, sc, cx + cw - 58.f, ry + 11.f, Font, CAP);
			}
		}

		// ================= LEFT: CAMERAS (top) =================
		if (PC && PC->NumCameras() > 0)
		{
			const int32 NC = FMath::Min(PC->NumCameras(), 2);
			const float tw = 120.f, th = 66.f, y0 = 66.f;
			for (int32 i = 0; i < PC->NumCameras() && i < NC; ++i)
			{
				AGreenhouseCamera* Cam = PC->GetCameraAt(i);
				const float x = 14.f + i*(tw+8.f);
				const bool active = (i == PC->CurrentCameraIndex());
				DrawRect(FLinearColor(0.02f,0.02f,0.03f,1.f), x, y0, tw, th);
				if (Cam && Cam->GetPreviewRT()) DrawTexture(Cam->GetPreviewRT(), x, y0, tw, th, 0.f,0.f,1.f,1.f);
				Outline(x, y0, tw, th, active ? Accent : Border, active ? 2.f : 1.f);
				DrawText(Cam ? Cam->RoomLabel : FString::Printf(TEXT("Cam %d"),i), active ? Amber : Prim, x + 5.f, y0 + th - 16.f, Font, CAP);
				AddHitBox(FVector2D(x,y0), FVector2D(tw,th), FName(*FString::Printf(TEXT("cam%d"),i)), true);
			}
		}

		// ================= LEFT-BOTTOM: MISSION LOG + OBJECTIVE =================
		{
			const float lw = 300.f, lx = 14.f, lh = 150.f, ly = SH - lh - 168.f;
			Card(lx, ly, lw, lh, CardBg);
			DrawText(TEXT("MISSION LOG"), Amber, lx + 14.f, ly + 8.f, Font, CAP);
			const int32 N = S->MissionLog.Num(), Show = FMath::Min(5, N);
			for (int32 i = 0; i < Show; ++i)
			{
				const FString& Line = S->MissionLog[N - Show + i];
				DrawText(Line, i == Show-1 ? Prim : Sec, lx + 14.f, ly + 30.f + i*22.f, Font, CAP);
			}
			// objective below
			const float oy = ly + lh + 8.f, oh = 62.f;
			Card(lx, oy, lw, oh, CardBg);
			DrawRect(Accent, lx, oy, 4.f, oh);
			DrawText(TEXT("OBJECTIVE"), Accent, lx + 14.f, oy + 8.f, Font, CAP);
			DrawWrapped(S->CurrentObjective(), Prim, lx + 14.f, oy + 26.f, lw - 26.f, CAP);
		}

		// ================= BOTTOM-CENTER: LIGHT + PLANTERS + CROP CARDS =================
		{
			const float py0 = SH - 150.f;
			// grow-light toggle
			const float gw = 220.f, gx = SW*0.5f - gw*0.5f;
			Card(gx, py0, gw, 34.f, CardBg);
			const float seg = (gw-6.f)/2.f;
			Button(gx+3.f,        py0+3.f, seg-2.f, 28.f, TEXT("Purple"), FString(), S->LedColor==ELedColor::Purple?PurpleC:FLinearColor(0.2f,0.12f,0.28f,0.7f), S->LedColor==ELedColor::Purple?Prim:Sec, FName("led_purple"), true, TEXT("Purple: more oxygen, steady growth (lettuce)."));
			Button(gx+3.f+seg,    py0+3.f, seg-2.f, 28.f, TEXT("White"),  FString(), S->LedColor==ELedColor::White ?WhiteC :FLinearColor(0.28f,0.26f,0.22f,0.7f), S->LedColor==ELedColor::White?AccTxt:Sec, FName("led_white"), true, TEXT("White: faster growth, less oxygen (potato)."));

			// planter chips
			const float cy = py0 + 40.f; const int32 NB = S->Beds.Num();
			const float chw = 48.f, cg = 8.f, tot = NB*chw + (NB-1)*cg, cx0 = SW*0.5f - tot*0.5f;
			for (int32 i = 0; i < NB; ++i)
			{
				const float x = cx0 + i*(chw+cg); const bool sel = (i==Sel);
				Card(x, cy, chw, 30.f, sel ? CardHi : CardBg);
				if (sel) Outline(x, cy, chw, 30.f, Accent, 2.f);
				DrawText(FString::Printf(TEXT("%d"), i), sel ? Accent : Sec, x + 8.f, cy + 6.f, Font, LBL);
				Circle(x + chw - 12.f, cy + 15.f, 4.f, BedStateColor(S->Beds[i], i));
				AddHitBox(FVector2D(x, cy), FVector2D(chw, 30.f), FName(*FString::Printf(TEXT("bed%d"), i)), true);
			}

			// selected-planter stage strip
			if (S->Beds.IsValidIndex(Sel))
			{
				const FPlantedBed& B = S->Beds[Sel];
				const float pw = 320.f, sx = SW*0.5f - pw*0.5f, sy = cy + 36.f;
				Card(sx, sy, pw, 40.f, CardBg);
				if (!B.bOccupied) DrawText(FString::Printf(TEXT("Planter %d - empty, plant a crop"), Sel), Muted, sx + 14.f, sy + 12.f, Font, SUB);
				else
				{
					const FCropStats& C = Cfg->GetCrop(B.Crop);
					const int32 stg = FMath::Clamp(CropStageIndex(C.StageEnds, B.Growth), 0, FMath::Max(0, C.StageNames.Num()-1));
					const FString Stage = B.bSpoiled ? FString::Printf(TEXT("Spoiled (%s)"), *C.SpoilName.ToString()) : (S->IsBedReady(Sel) ? FString::Printf(TEXT("READY  %d sol left"), S->SolsLeftToHarvest(Sel)) : (C.StageNames.IsValidIndex(stg) ? C.StageNames[stg].ToString() : FString(CropName(B.Crop))));
					const FLinearColor pc = B.bSpoiled ? Red : (S->IsBedReady(Sel) ? Green : Amber);
					DrawText(FString::Printf(TEXT("%s  -  %s"), CropName(B.Crop), *Stage), pc, sx + 14.f, sy + 5.f, Font, SUB);
					Bar(sx + 14.f, sy + 26.f, pw - 28.f, 6.f, B.Growth, pc);
				}
			}
		}
	}

	// ================= BOTTOM ACTION DOCK =================
	if (!bEnded && !bCustomDash)
	{
		const float M = 14.f, availW = SW - 2.f*M, barH = 58.f, barY = SH - barH - 10.f;
		Card(M - 4.f, barY - 6.f, availW + 8.f, barH + 2.f, CardBg);
		const bool bOcc = S->Beds.IsValidIndex(Sel) && S->Beds[Sel].bOccupied;
		const bool bReady = S->IsBedReady(Sel);
		const bool bWater = bOcc && !S->Beds[Sel].bSpoiled && S->Water >= Cfg->WaterPlantWaterCost && S->Power >= Cfg->WaterPlantPowerCost && bLive;
		const bool bHarv = bOcc && bLive;
		const bool bIce = bLive && S->Power >= Cfg->MineIcePowerCost;
		const bool bEle = bLive && S->Water >= Cfg->ElectrolyzeWaterCost && S->Power >= Cfg->ElectrolyzePowerCost;
		const FLinearColor AF(0.12f,0.11f,0.09f,0.98f);
		const FLinearColor HF = bReady ? FLinearColor(0.16f,0.30f,0.16f,0.99f) : AF;
		const float endW = 168.f, g = 8.f, dockW = availW - endW - g;
		const int32 NBD = 5; const float bw = (dockW - (NBD-1)*g)/NBD, bh = 44.f, ry = barY + 5.f;
		auto AB = [&](int32 i, const FString& lbl, const FString& sub, const FLinearColor& c, FName id, bool en, const FString& tip){ Button(M + i*(bw+g), ry, bw, bh, lbl, sub, c, Prim, id, en, tip); };
		AB(0, TEXT("Plant"), TEXT("potato / lettuce"), AF, FName("plant_potato"), bLive && !bOcc, TEXT("Plant a crop in the selected planter."));
		AB(1, TEXT("Water"), FString::Printf(TEXT("-%d H2O"), FMath::RoundToInt(Cfg->WaterPlantWaterCost)), AF, FName("act_water"), bWater, TEXT("Water the selected planter. Neglected plants die."));
		AB(2, bReady ? TEXT("Harvest !") : TEXT("Harvest"), TEXT("[E]"), HF, FName("act_harvest"), bHarv, TEXT("Collect. Early = partial; too late = spoiled."));
		AB(3, TEXT("Mine ice"), FString::Printf(TEXT("+%d -%d Pwr"), FMath::RoundToInt(Cfg->MineIceWater), FMath::RoundToInt(Cfg->MineIcePowerCost)), AF, FName("act_ice"), bIce, TEXT("Water from ice. Costs power."));
		AB(4, TEXT("Electrolyze"), FString::Printf(TEXT("+%d O2"), FMath::RoundToInt(Cfg->ElectrolyzeOxygen)), AF, FName("act_electro"), bEle, TEXT("Water into oxygen. Costs water + power."));
		Button(SW - M - endW, ry, endW, bh, TEXT("END DAY"), TEXT("[Enter]"), Accent, AccTxt, FName("advance"), bLive, TEXT("Resolve the sol: crew eats, crops grow, weather turns."));
	}

	// ================= EVENT CARD =================
	if (S->bEventActive && !bCustomEvent)
	{
		const float w = 720.f, h = 292.f, ex = SW*0.5f - w*0.5f, ey = SH*0.38f - h*0.5f;
		Card(ex, ey, w, h, FLinearColor(0.10f,0.07f,0.05f,0.99f));
		DrawRect(Accent, ex + 12.f, ey + 12.f, 4.f, 22.f);
		DrawText(S->CurrentEvent.bSingleChoice ? TEXT("STORY") : TEXT("DECISION"), Amber, ex + 26.f, ey + 16.f, Font, CAP);
		DrawText(S->CurrentEvent.Speaker.ToString(), O2C, ex + 26.f, ey + 38.f, Font, VAL);
		DrawWrapped(S->CurrentEvent.Situation.ToString(), Prim, ex + 26.f, ey + 76.f, w - 52.f, LBL);
		if (S->CurrentEvent.bSingleChoice)
			Button(ex + 26.f, ey + h - 62.f, w - 52.f, 44.f, S->CurrentEvent.ChoiceA.Label.ToString(), FString(), FLinearColor(0.14f,0.26f,0.16f,0.98f), Prim, FName("evt_a"), true, FString());
		else
		{
			Button(ex + 26.f, ey + h - 96.f, w - 52.f, 42.f, FString::Printf(TEXT("A:  %s"), *S->CurrentEvent.ChoiceA.Label.ToString()), EffectArrows(S->CurrentEvent.ChoiceA.Effect), FLinearColor(0.14f,0.26f,0.16f,0.98f), Prim, FName("evt_a"), true, FString());
			Button(ex + 26.f, ey + h - 48.f, w - 52.f, 38.f, FString::Printf(TEXT("B:  %s"), *S->CurrentEvent.ChoiceB.Label.ToString()), EffectArrows(S->CurrentEvent.ChoiceB.Effect), FLinearColor(0.28f,0.20f,0.10f,0.98f), Prim, FName("evt_b"), true, FString());
		}
	}

	// ================= WIN / LOSE =================
	if (bEnded)
	{
		const bool bWon = S->State == EGameState::Won;
		DrawRect(FLinearColor(0.04f,0.03f,0.02f,0.9f), 0.f, 0.f, SW, SH);
		const float cw = 430.f, cx = SW*0.5f - cw*0.5f, cy = SH*0.5f - 150.f;
		Card(cx, cy, cw, 300.f, CardHi);
		if (LogoTex) DrawTexture(LogoTex, cx + cw*0.5f - 90.f, cy + 14.f, 180.f, 56.f, 0.f,0.f,1.f,1.f);
		DrawText(bWon ? TEXT("BUSTAN ENDURES") : TEXT("THE GARDEN FELL"), bWon ? Green : Red, cx + 30.f, cy + (LogoTex?78.f:26.f), Font, BIG);
		DrawText(FString::Printf(TEXT("Sols survived:  %d / %d"), FMath::Min(S->Sol - 1, Cfg->TotalSols), Cfg->TotalSols), Prim, cx + 30.f, cy + 120.f, Font, VAL);
		DrawText(FString::Printf(TEXT("Harvests:  %d       Crops lost:  %d"), S->Harvests, S->Spoiled), Sec, cx + 30.f, cy + 152.f, Font, LBL);
		DrawText(TEXT("RANK"), Muted, cx + 30.f, cy + 188.f, Font, SUB);
		DrawText(S->ResultRank(), Amber, cx + 108.f, cy + 178.f, Font, BIG);
		Button(cx + 30.f, cy + 240.f, cw - 60.f, 44.f, TEXT("RESTART RUN"), FString(), Accent, AccTxt, FName("restart"), true, FString());
	}

	// ================= TUTORIAL =================
	if (PC && PC->TutorialActive() && !bEnded)
	{
		const float bw = 780.f, bx = SW*0.5f - bw*0.5f, by = SH*0.5f - 40.f, bh = 66.f;
		Card(bx, by, bw, bh, FLinearColor(0.10f,0.07f,0.05f,0.98f));
		DrawRect(FLinearColor(Accent.R, Accent.G, Accent.B, 0.5f+0.4f*pulse), bx + 12.f, by + 10.f, 4.f, bh - 20.f);
		DrawText(FString::Printf(TEXT("TUTORIAL  %d/6"), TStep + 1), Amber, bx + 24.f, by + 8.f, Font, CAP);
		DrawWrapped(PC->TutorialPrompt(), Prim, bx + 24.f, by + 26.f, bw - 220.f, SUB);
		Button(bx + bw - 92.f, by + bh - 30.f, 82.f, 24.f, TEXT("Skip"), FString(), FLinearColor(0.30f,0.12f,0.10f,0.98f), Prim, FName("tut_skip"), true, FString());
		if (PC->TutorialWantsNext()) Button(bx + bw - 184.f, by + bh - 30.f, 86.f, 24.f, TEXT("Next"), FString(), FLinearColor(0.14f,0.28f,0.16f,0.98f), Prim, FName("tut_next"), true, FString());
	}

	// ================= FLOATERS =================
	for (int32 i = Floaters.Num() - 1; i >= 0; --i)
	{
		FFloatText& F = Floaters[i];
		F.Life -= dt; F.Y += 24.f * dt;
		if (F.Life <= 0.f) { Floaters.RemoveAt(i); continue; }
		FLinearColor C = F.Color; C.A = FMath::Clamp(F.Life / 1.3f, 0.f, 1.f);
		DrawText(F.Text, C, F.X, F.Y, Font, VAL);
	}

	// ================= TOOLTIP =================
	if (FrameHover == HoverBox) HoverTime += dt; else { HoverBox = FrameHover; HoverTime = 0.f; }
	if (HoverTime > 0.6f && !FrameTip.IsEmpty() && mx > 0.f)
	{
		const float tw = 280.f, tx = FMath::Min(mx + 16.f, SW - tw - 8.f), ty = my + 18.f;
		Card(tx, ty, tw, 46.f, FLinearColor(0.05f,0.055f,0.075f,0.99f));
		DrawWrapped(FrameTip, Prim, tx + 10.f, ty + 8.f, tw - 20.f, CAP);
	}
}