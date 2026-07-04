// Copyright Epic Games, Inc. All Rights Reserved.
// "Bustan" HUD — one cohesive slate-blue + grey palette, soft sky-blue accent, larger type,
// crisp square icons, world-anchored labels. Every art slot has a code-drawn fallback.

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
	UTexture2D* ResTex[4] = { L(Cfg->IconOxygen), L(Cfg->IconWater), L(Cfg->IconFood), L(Cfg->IconPower) };
	UTexture2D* PanelTex  = L(Cfg->PanelFrame);
	UTexture2D* ButtonTex = L(Cfg->ButtonFrame);
	UTexture2D* VignetteTex = L(Cfg->VignetteOverlay);
	UTexture2D* LogoTex   = L(Cfg->TitleLogo);

	AMarsGreenhousePlayerController* PC = Cast<AMarsGreenhousePlayerController>(PlayerOwner);
	const bool bCustomDash  = PC && PC->HasCustomDashboard();
	const bool bCustomEvent = PC && PC->HasCustomEventUI();

	const float SW = Canvas->SizeX, SH = Canvas->SizeY;
	const float dt = W->GetDeltaSeconds();
	const float Now = W->GetTimeSeconds();
	const float pulse = 0.5f + 0.5f * FMath::Sin(Now * 3.5f);

	// type scale (~18% larger than before)
	const float FS = 1.18f;
	const float CAP = 0.82f*FS, SUB = 0.92f*FS, LBL = 1.14f*FS, VAL = 1.5f*FS, BIG = 2.0f*FS;

	// ---- one cohesive slate-blue + grey palette (soft sky-blue accent) ----
	const FLinearColor Prim  (0.90f, 0.93f, 0.97f);
	const FLinearColor Sec   (0.62f, 0.68f, 0.77f);
	const FLinearColor Muted (0.44f, 0.49f, 0.58f);
	const FLinearColor Accent(0.52f, 0.74f, 0.94f);   // soft sky blue
	const FLinearColor AccTxt(0.04f, 0.09f, 0.15f);
	const FLinearColor O2C   (0.44f, 0.82f, 0.82f);
	const FLinearColor H2OC  (0.46f, 0.68f, 0.95f);
	const FLinearColor FoodC (0.56f, 0.82f, 0.56f);
	const FLinearColor PwrC  (0.96f, 0.74f, 0.42f);
	const FLinearColor Red   (0.94f, 0.48f, 0.48f);
	const FLinearColor Green (0.52f, 0.84f, 0.58f);
	const FLinearColor Warn  (0.96f, 0.76f, 0.46f);   // soft amber for warnings / growing
	const FLinearColor PurpleC(0.66f, 0.50f, 0.98f);
	const FLinearColor WhiteC (0.94f, 0.96f, 1.00f);
	const FLinearColor CardBg(0.105f, 0.135f, 0.185f, 0.94f);   // slate blue-grey
	const FLinearColor CardHi(0.15f, 0.19f, 0.26f, 0.96f);
	const FLinearColor Btn   (0.17f, 0.21f, 0.28f, 0.96f);      // uniform button fill
	const FLinearColor IconBg(0.105f, 0.135f, 0.185f, 1.f);
	const FLinearColor Border(0.55f, 0.72f, 0.94f, 0.20f);
	const FLinearColor BarBg (1.f, 1.f, 1.f, 0.10f);

	float mx = -1.f, my = -1.f;
	if (PC) PC->GetMousePosition(mx, my);

	auto MouseIn  = [&](float x, float y, float w, float h){ return mx>=x && mx<=x+w && my>=y && my<=y+h; };
	auto Brighten = [&](const FLinearColor& c){ return FLinearColor(FMath::Min(1.f,c.R*1.28f), FMath::Min(1.f,c.G*1.28f), FMath::Min(1.f,c.B*1.28f), c.A); };
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
	// crisp rounded-square icon (straight edges read clean; no jaggy circles)
	auto ResIcon = [&](int32 i, float x, float y, float s, const FLinearColor& col)
	{
		if (ResTex[i]) { DrawTexture(ResTex[i], x, y, s, s, 0.f,0.f,1.f,1.f, col); return; }
		RoundRect(x + s*0.14f, y + s*0.14f, s*0.72f, s*0.72f, s*0.24f, col);
		RoundRect(x + s*0.34f, y + s*0.34f, s*0.32f, s*0.32f, s*0.12f, IconBg);
	};
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
		const FLinearColor bg = !enabled ? FLinearColor(0.10f,0.12f,0.16f,0.85f) : (hov ? Brighten(fill) : fill);
		if (ButtonTex) NineSlice(ButtonTex, x, y, w, h, Cfg->ButtonMargin, bg);
		else { if (hov) RoundRect(x-2.f,y-2.f,w+4.f,h+4.f,9.f,Accent); RoundRect(x, y, w, h, 7.f, bg); }
		DrawText(label, enabled ? txt : Muted, x + 13.f, y + (sub.IsEmpty() ? h*0.5f - 10.f : 6.f), Font, LBL);
		if (!sub.IsEmpty()) DrawText(sub, enabled ? FLinearColor(txt.R*0.82f,txt.G*0.82f,txt.B*0.82f,0.92f) : Muted, x + 13.f, y + h - 19.f, Font, CAP);
		if (enabled) { AddHitBox(FVector2D(x,y), FVector2D(w,h), id, true); if (hov) { FrameHover = id; FrameTip = tip; } }
	};
	auto DrawWrapped = [&](const FString& Text, const FLinearColor& Col, float x, float y, float maxW, float scale) -> float
	{
		TArray<FString> Words; Text.ParseIntoArray(Words, TEXT(" "));
		const float charW = 9.2f * scale; const int32 maxChars = FMath::Max(10, (int32)(maxW / charW));
		FString Ln; float ly = y;
		for (const FString& Wd : Words)
		{
			if (Ln.Len() + Wd.Len() + 1 > maxChars) { DrawText(Ln, Col, x, ly, Font, scale); ly += 20.f * scale; Ln = Wd; }
			else { Ln = Ln.IsEmpty() ? Wd : Ln + TEXT(" ") + Wd; }
		}
		if (!Ln.IsEmpty()) { DrawText(Ln, Col, x, ly, Font, scale); ly += 20.f * scale; }
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
		return Warn;
	};

	if (VignetteTex) DrawTexture(VignetteTex, 0.f, 0.f, SW, SH, 0.f,0.f,1.f,1.f, FLinearColor(1,1,1,0.5f));
	if (S->DustLevel > 0.f) DrawRect(FLinearColor(0.45f,0.30f,0.14f, S->DustLevel*0.26f), 0.f, 0.f, SW, SH);

	// ================= WORLD LABELS + planter highlights + on-planter menus =================
	if (!bEnded && !bCustomDash)
	{
		auto WorldTag = [&](const FVector& Loc, const FString& Txt, const FLinearColor& col, bool bold)
		{
			const FVector Scr = Project(Loc);
			if (Scr.Z <= 0.f) return FVector2D(-1.f,-1.f);
			const float tw = 14.f + Txt.Len() * 9.0f;
			const float tx = Scr.X - tw*0.5f, ty = Scr.Y;
			Card(tx, ty, tw, 26.f, CardBg);
			if (bold) Outline(tx, ty, tw, 26.f, col, 1.f);
			DrawText(Txt, bold ? col : Prim, tx + 9.f, ty + 5.f, Font, CAP);
			return FVector2D(Scr.X, Scr.Y);
		};

		TArray<AActor*> LabelActors;
		UGameplayStatics::GetAllActorsOfClass(this, AGreenhouseLabel::StaticClass(), LabelActors);
		for (AActor* A : LabelActors)
			if (AGreenhouseLabel* Lab = Cast<AGreenhouseLabel>(A))
				WorldTag(Lab->GetActorLocation() + FVector(0,0,Lab->HeightOffset), Lab->LabelText, Accent, false);

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
			const FLinearColor oc = sel ? Accent : (ready ? FLinearColor(hc.R,hc.G,hc.B, 0.5f+0.5f*pulse) : (hov ? Prim : FLinearColor(hc.R,hc.G,hc.B,0.55f)));
			Outline(px-bxs, py-bys, bxs*2, bys*2, oc, (sel||hov)?2.5f:1.f);
			Circle(px, py-bys-6.f, 5.f, hc);
			DrawText(FString::Printf(TEXT("Planter %d"), bi), sel ? Accent : Sec, px-bxs, py+bys+4.f, Font, CAP);
			AddHitBox(FVector2D(px-bxs, py-bys), FVector2D(bxs*2, bys*2), FName(*FString::Printf(TEXT("bed%d"), bi)), true);

			if (sel && bLive)
			{
				const float mw = 168.f, ax = px - mw*0.5f, ay = py - bys - 82.f;
				if (!B.bOccupied)
				{
					const float hw = (mw-6.f)*0.5f;
					Button(ax,        ay, hw, 32.f, TEXT("Potato"),  FString(), Btn, Prim, FName("plant_potato"),  true, TEXT("Potato - slow, high food, likes white light."));
					Button(ax+hw+6.f, ay, hw, 32.f, TEXT("Lettuce"), FString(), Btn, Prim, FName("plant_lettuce"), true, TEXT("Lettuce - fast, good oxygen, likes purple light."));
				}
				else if (B.bSpoiled) Button(ax, ay, mw, 32.f, TEXT("Clear planter"), FString(), Btn, Red, FName("act_harvest"), true, TEXT("Remove the spoiled crop."));
				else if (ready)      Button(ax, ay, mw, 32.f, TEXT("Harvest now"),   FString(), Accent, AccTxt, FName("act_harvest"), true, TEXT("Collect before it spoils."));
				else
				{
					const float hw = (mw-6.f)*0.5f;
					Button(ax,        ay, hw, 32.f, TEXT("Water"),   FString(), Btn, Prim, FName("act_water"),   true, TEXT("Restore health."));
					Button(ax+hw+6.f, ay, hw, 32.f, TEXT("Harvest"), FString(), Btn, Prim, FName("act_harvest"), true, TEXT("Harvest early (partial)."));
				}
			}
		}

		TArray<AActor*> LedActors;
		UGameplayStatics::GetAllActorsOfClass(this, AGreenhouseLED::StaticClass(), LedActors);
		for (AActor* A : LedActors)
		{
			const FVector2D P = WorldTag(A->GetActorLocation() + FVector(0,0,50.f), TEXT("Grow Lights"), S->LedColor==ELedColor::White?WhiteC:PurpleC, true);
			if (P.X < 0.f) continue;
			const bool bNear = FMath::Abs(P.X-mx) < 92.f && FMath::Abs(P.Y-my) < 72.f;
			if (bNear && bLive)
			{
				const float bw = 90.f, ax = P.X - (bw*2+6.f)*0.5f, ay = P.Y - 42.f;
				Button(ax,        ay, bw, 30.f, TEXT("Purple"), FString(), S->LedColor==ELedColor::Purple?PurpleC:Btn, S->LedColor==ELedColor::Purple?AccTxt:Prim, FName("led_purple"), true, TEXT("More oxygen, steady growth."));
				Button(ax+bw+6.f, ay, bw, 30.f, TEXT("White"),  FString(), S->LedColor==ELedColor::White ?WhiteC :Btn, S->LedColor==ELedColor::White?AccTxt:Prim,  FName("led_white"),  true, TEXT("Faster growth, less oxygen."));
				break;
			}
		}
	}

	if (!bCustomDash)
	{
		// ================= TOP RESOURCE BAR =================
		{
			const TCHAR* RN[4] = { TEXT("Oxygen"), TEXT("Water"), TEXT("Food"), TEXT("Power") };
			const FLinearColor RC[4] = { O2C, H2OC, FoodC, PwrC };
			const float bw = 250.f, bh = 50.f; float bx = 14.f, by = 10.f;
			for (int32 i = 0; i < 4; ++i)
			{
				const float Val = Meters[i];
				DispVal[i] = FMath::FInterpTo(DispVal[i], Val, dt, 8.f);
				if (FMath::Abs(Val - LastVal[i]) >= 1.f) { LastDelta[i] = Val - LastVal[i]; Floaters.Add({ FString::Printf(TEXT("%+d"), FMath::RoundToInt(LastDelta[i])), LastDelta[i]>0?Green:Red, bx + bw*0.5f, by + bh + 4.f, 1.3f }); }
				LastVal[i] = Val;
				const bool low = Val <= 15.f;
				Card(bx, by, bw, bh, low ? FLinearColor(0.20f,0.10f,0.12f,0.95f) : CardBg);
				ResIcon(i, bx + 12.f, by + 12.f, 26.f, RC[i]);
				DrawText(RN[i], Sec, bx + 50.f, by + 6.f, Font, CAP);
				DrawText(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(DispVal[i])), low ? Red : Prim, bx + bw - 54.f, by + 5.f, Font, SUB);
				SegMeter(bx + 50.f, by + 28.f, bw - 64.f, 11.f, DispVal[i]*0.01f, low ? Red : RC[i]);
				bx += bw + 8.f;
			}
			const float pw = 152.f, pxp = SW - pw - 14.f;
			Card(pxp, by, pw, bh, CardBg);
			DrawText(TEXT("SOL"), Muted, pxp + 16.f, by + 7.f, Font, CAP);
			DrawText(FString::Printf(TEXT("%02d"), S->Sol), Accent, pxp + 16.f, by + 21.f, Font, VAL);
			DrawText(FString::Printf(TEXT("/ %d"), Cfg->TotalSols), Sec, pxp + 66.f, by + 28.f, Font, CAP);
			DrawText(bLive ? TEXT("DAY") : TEXT("..."), Sec, pxp + pw - 48.f, by + 9.f, Font, CAP);
		}

		{
			const float sbw = 520.f, sx = SW*0.5f - sbw*0.5f, sy = 66.f;
			if (S->DustLevel > 0.f) { Card(sx, sy, sbw, 34.f, FLinearColor(0.20f,0.12f,0.10f,0.96f)); DrawText(FString::Printf(TEXT("DUST STORM   solar -%d%% this sol"), FMath::RoundToInt(S->DustLevel*100.f)), Warn, sx + 16.f, sy + 8.f, Font, SUB); }
			else if (S->NextDust > 0.f) { Card(sx, sy, sbw, 30.f, CardBg); DrawText(FString::Printf(TEXT("Storm forecast next sol (solar -%d%%) - bank power"), FMath::RoundToInt(S->NextDust*100.f)), Warn, sx + 16.f, sy + 6.f, Font, SUB); }
		}

		// ================= RIGHT: CREW STATUS =================
		{
			const float cw = 270.f, cx = SW - cw - 14.f, cyTop = 68.f;
			const int32 NCrew = Cfg->Crew.Num();
			const float rowH = 46.f, ph = 30.f + NCrew * rowH;
			Card(cx, cyTop, cw, ph, CardBg);
			DrawText(TEXT("CREW STATUS"), Accent, cx + 16.f, cyTop + 9.f, Font, CAP);
			for (int32 i = 0; i < NCrew; ++i)
			{
				const float ry = cyTop + 30.f + i * rowH;
				UTexture2D* Portrait = Cfg->CrewPortraits.IsValidIndex(i) ? L(Cfg->CrewPortraits[i]) : nullptr;
				if (Portrait) DrawTexture(Portrait, cx + 12.f, ry + 3.f, 34.f, 34.f, 0.f,0.f,1.f,1.f);
				else { RoundRect(cx + 12.f, ry + 3.f, 34.f, 34.f, 8.f, FLinearColor(0.16f,0.20f,0.27f,1.f)); DrawText(Cfg->Crew[i].Name.ToString().Left(1), Accent, cx + 22.f, ry + 10.f, Font, SUB); }
				DrawText(Cfg->Crew[i].Name.ToString(), Prim, cx + 54.f, ry + 3.f, Font, CAP);
				DrawText(Cfg->Crew[i].Role.ToString(), Muted, cx + 54.f, ry + 21.f, Font, CAP);
				const FString St = S->GetCrewStatus(i);
				const FLinearColor sc = (St == TEXT("OK")) ? Green : (St == TEXT("Critical") ? Red : Warn);
				DrawText(St, sc, cx + cw - 62.f, ry + 12.f, Font, CAP);
			}
		}

		// ================= LEFT: CAMERAS =================
		if (PC && PC->NumCameras() > 0)
		{
			const int32 NC = FMath::Min(PC->NumCameras(), 2);
			const float tw = 128.f, th = 70.f, y0 = 68.f;
			for (int32 i = 0; i < PC->NumCameras() && i < NC; ++i)
			{
				AGreenhouseCamera* Cam = PC->GetCameraAt(i);
				const float x = 14.f + i*(tw+8.f);
				const bool active = (i == PC->CurrentCameraIndex());
				DrawRect(FLinearColor(0.02f,0.03f,0.05f,1.f), x, y0, tw, th);
				if (Cam && Cam->GetPreviewRT()) DrawTexture(Cam->GetPreviewRT(), x, y0, tw, th, 0.f,0.f,1.f,1.f);
				Outline(x, y0, tw, th, active ? Accent : Border, active ? 2.f : 1.f);
				DrawText(Cam ? Cam->RoomLabel : FString::Printf(TEXT("Cam %d"),i), active ? Accent : Prim, x + 6.f, y0 + th - 18.f, Font, CAP);
				AddHitBox(FVector2D(x,y0), FVector2D(tw,th), FName(*FString::Printf(TEXT("cam%d"),i)), true);
			}
		}

		// ================= LEFT-BOTTOM: MISSION LOG + OBJECTIVE =================
		{
			const float lw = 340.f, lx = 14.f, lh = 162.f, ly = SH - lh - 176.f;
			Card(lx, ly, lw, lh, CardBg);
			DrawText(TEXT("MISSION LOG"), Accent, lx + 16.f, ly + 9.f, Font, CAP);
			const int32 N = S->MissionLog.Num(), Show = FMath::Min(5, N);
			for (int32 i = 0; i < Show; ++i)
				DrawText(S->MissionLog[N - Show + i], i == Show-1 ? Prim : Sec, lx + 16.f, ly + 34.f + i*24.f, Font, CAP);
			const float oy = ly + lh + 8.f, oh = 68.f;
			Card(lx, oy, lw, oh, CardBg);
			DrawRect(Accent, lx, oy, 4.f, oh);
			DrawText(TEXT("OBJECTIVE"), Accent, lx + 16.f, oy + 9.f, Font, CAP);
			DrawWrapped(S->CurrentObjective(), Prim, lx + 16.f, oy + 30.f, lw - 28.f, CAP);
		}

		// ================= BOTTOM-CENTER: LIGHT + PLANTERS + STAGE =================
		{
			const float py0 = SH - 158.f;
			const float gw = 236.f, gx = SW*0.5f - gw*0.5f;
			Card(gx, py0, gw, 36.f, CardBg);
			const float seg = (gw-6.f)/2.f;
			Button(gx+3.f,     py0+3.f, seg-2.f, 30.f, TEXT("Purple"), FString(), S->LedColor==ELedColor::Purple?PurpleC:Btn, S->LedColor==ELedColor::Purple?AccTxt:Sec, FName("led_purple"), true, TEXT("Purple: more oxygen, steady growth."));
			Button(gx+3.f+seg, py0+3.f, seg-2.f, 30.f, TEXT("White"),  FString(), S->LedColor==ELedColor::White ?WhiteC :Btn, S->LedColor==ELedColor::White?AccTxt:Sec,  FName("led_white"), true, TEXT("White: faster growth, less oxygen."));
			DrawText(TEXT("Grow-light"), Muted, gx, py0 + 40.f, Font, CAP);

			const float cy = py0 + 62.f; const int32 NB = S->Beds.Num();
			const float chw = 56.f, cg = 8.f, tot = NB*chw + (NB-1)*cg, cx0 = SW*0.5f - tot*0.5f;
			for (int32 i = 0; i < NB; ++i)
			{
				const float x = cx0 + i*(chw+cg); const bool sel = (i==Sel);
				Card(x, cy, chw, 32.f, sel ? CardHi : CardBg);
				if (sel) Outline(x, cy, chw, 32.f, Accent, 2.f);
				DrawText(FString::Printf(TEXT("%d"), i), sel ? Accent : Sec, x + 9.f, cy + 7.f, Font, LBL);
				Circle(x + chw - 12.f, cy + 16.f, 4.f, BedStateColor(S->Beds[i], i));
				AddHitBox(FVector2D(x, cy), FVector2D(chw, 32.f), FName(*FString::Printf(TEXT("bed%d"), i)), true);
			}

			if (S->Beds.IsValidIndex(Sel))
			{
				const FPlantedBed& B = S->Beds[Sel];
				const float pw = 340.f, sx = SW*0.5f - pw*0.5f, sy = cy + 38.f;
				Card(sx, sy, pw, 42.f, CardBg);
				if (!B.bOccupied) DrawText(FString::Printf(TEXT("Planter %d - empty, plant a crop"), Sel), Muted, sx + 16.f, sy + 13.f, Font, SUB);
				else
				{
					const FCropStats& C = Cfg->GetCrop(B.Crop);
					const int32 stg = FMath::Clamp(CropStageIndex(C.StageEnds, B.Growth), 0, FMath::Max(0, C.StageNames.Num()-1));
					const FString Stage = B.bSpoiled ? FString::Printf(TEXT("Spoiled (%s)"), *C.SpoilName.ToString()) : (S->IsBedReady(Sel) ? FString::Printf(TEXT("READY  %d sol left"), S->SolsLeftToHarvest(Sel)) : (C.StageNames.IsValidIndex(stg) ? C.StageNames[stg].ToString() : FString(CropName(B.Crop))));
					const FLinearColor pc = B.bSpoiled ? Red : (S->IsBedReady(Sel) ? Green : Warn);
					DrawText(FString::Printf(TEXT("%s  -  %s"), CropName(B.Crop), *Stage), pc, sx + 16.f, sy + 5.f, Font, SUB);
					Bar(sx + 16.f, sy + 28.f, pw - 32.f, 7.f, B.Growth, pc);
				}
			}
		}
	}

	// ================= BOTTOM ACTION DOCK =================
	if (!bEnded && !bCustomDash)
	{
		const float M = 14.f, availW = SW - 2.f*M, barH = 62.f, barY = SH - barH - 10.f;
		Card(M - 4.f, barY - 6.f, availW + 8.f, barH + 2.f, CardBg);
		const bool bOcc = S->Beds.IsValidIndex(Sel) && S->Beds[Sel].bOccupied;
		const bool bReady = S->IsBedReady(Sel);
		const bool bWater = bOcc && !S->Beds[Sel].bSpoiled && S->Water >= Cfg->WaterPlantWaterCost && S->Power >= Cfg->WaterPlantPowerCost && bLive;
		const bool bHarv = bOcc && bLive;
		const bool bIce = bLive && S->Power >= Cfg->MineIcePowerCost;
		const bool bEle = bLive && S->Water >= Cfg->ElectrolyzeWaterCost && S->Power >= Cfg->ElectrolyzePowerCost;
		const FLinearColor HF = bReady ? Accent : Btn;
		const FLinearColor HT = bReady ? AccTxt : Prim;
		const float endW = 196.f, g = 8.f, dockW = availW - endW - g;
		const int32 NBD = 5; const float bw = (dockW - (NBD-1)*g)/NBD, bh = 48.f, ry = barY + 5.f;
		auto AB = [&](int32 i, const FString& lbl, const FString& sub, const FLinearColor& c, const FLinearColor& tc, FName id, bool en, const FString& tip){ Button(M + i*(bw+g), ry, bw, bh, lbl, sub, c, tc, id, en, tip); };
		AB(0, TEXT("Plant"), TEXT("potato / lettuce"), Btn, Prim, FName("plant_potato"), bLive && !bOcc, TEXT("Plant a crop in the selected planter."));
		AB(1, TEXT("Water"), FString::Printf(TEXT("-%d H2O"), FMath::RoundToInt(Cfg->WaterPlantWaterCost)), Btn, Prim, FName("act_water"), bWater, TEXT("Water the selected planter. Neglected plants die."));
		AB(2, bReady ? TEXT("Harvest !") : TEXT("Harvest"), TEXT("[E]"), HF, HT, FName("act_harvest"), bHarv, TEXT("Collect. Early = partial; too late = spoiled."));
		AB(3, TEXT("Mine ice"), FString::Printf(TEXT("+%d -%d Pwr"), FMath::RoundToInt(Cfg->MineIceWater), FMath::RoundToInt(Cfg->MineIcePowerCost)), Btn, Prim, FName("act_ice"), bIce, TEXT("Water from ice. Costs power."));
		AB(4, TEXT("Electrolyze"), FString::Printf(TEXT("+%d O2"), FMath::RoundToInt(Cfg->ElectrolyzeOxygen)), Btn, Prim, FName("act_electro"), bEle, TEXT("Water into oxygen. Costs water + power."));
		Button(SW - M - endW, ry, endW, bh, TEXT("END DAY"), TEXT("[Enter]"), Accent, AccTxt, FName("advance"), bLive, TEXT("Resolve the sol: crew eats, crops grow, weather turns."));
	}

	// ================= EVENT CARD =================
	if (S->bEventActive && !bCustomEvent)
	{
		const float w = 760.f, h = 300.f, ex = SW*0.5f - w*0.5f, ey = SH*0.38f - h*0.5f;
		Card(ex, ey, w, h, CardHi);
		DrawRect(Accent, ex + 12.f, ey + 12.f, 4.f, 24.f);
		DrawText(S->CurrentEvent.bSingleChoice ? TEXT("STORY") : TEXT("DECISION"), Accent, ex + 28.f, ey + 16.f, Font, CAP);
		DrawText(S->CurrentEvent.Speaker.ToString(), Prim, ex + 28.f, ey + 40.f, Font, VAL);
		DrawWrapped(S->CurrentEvent.Situation.ToString(), Prim, ex + 28.f, ey + 80.f, w - 56.f, LBL);
		if (S->CurrentEvent.bSingleChoice)
			Button(ex + 28.f, ey + h - 64.f, w - 56.f, 46.f, S->CurrentEvent.ChoiceA.Label.ToString(), FString(), Accent, AccTxt, FName("evt_a"), true, FString());
		else
		{
			Button(ex + 28.f, ey + h - 100.f, w - 56.f, 44.f, FString::Printf(TEXT("A:  %s"), *S->CurrentEvent.ChoiceA.Label.ToString()), EffectArrows(S->CurrentEvent.ChoiceA.Effect), Btn, Prim, FName("evt_a"), true, FString());
			Button(ex + 28.f, ey + h - 50.f, w - 56.f, 40.f, FString::Printf(TEXT("B:  %s"), *S->CurrentEvent.ChoiceB.Label.ToString()), EffectArrows(S->CurrentEvent.ChoiceB.Effect), Btn, Prim, FName("evt_b"), true, FString());
		}
	}

	// ================= WIN / LOSE =================
	if (bEnded)
	{
		const bool bWon = S->State == EGameState::Won;
		DrawRect(FLinearColor(0.03f,0.04f,0.06f,0.92f), 0.f, 0.f, SW, SH);
		const float cw = 440.f, cx = SW*0.5f - cw*0.5f, cy = SH*0.5f - 155.f;
		Card(cx, cy, cw, 310.f, CardHi);
		if (LogoTex) DrawTexture(LogoTex, cx + cw*0.5f - 90.f, cy + 16.f, 180.f, 56.f, 0.f,0.f,1.f,1.f);
		DrawText(bWon ? TEXT("BUSTAN ENDURES") : TEXT("THE GARDEN FELL"), bWon ? Green : Red, cx + 32.f, cy + (LogoTex?82.f:28.f), Font, BIG);
		DrawText(FString::Printf(TEXT("Sols survived:  %d / %d"), FMath::Min(S->Sol - 1, Cfg->TotalSols), Cfg->TotalSols), Prim, cx + 32.f, cy + 126.f, Font, VAL);
		DrawText(FString::Printf(TEXT("Harvests:  %d       Crops lost:  %d"), S->Harvests, S->Spoiled), Sec, cx + 32.f, cy + 158.f, Font, LBL);
		DrawText(TEXT("RANK"), Muted, cx + 32.f, cy + 196.f, Font, SUB);
		DrawText(S->ResultRank(), Accent, cx + 116.f, cy + 186.f, Font, BIG);
		Button(cx + 32.f, cy + 248.f, cw - 64.f, 46.f, TEXT("RESTART RUN"), FString(), Accent, AccTxt, FName("restart"), true, FString());
	}

	// ================= TUTORIAL =================
	if (PC && PC->TutorialActive() && !bEnded)
	{
		const float bw = 820.f, bx = SW*0.5f - bw*0.5f, by = SH*0.5f - 42.f, bh = 70.f;
		Card(bx, by, bw, bh, CardHi);
		DrawRect(FLinearColor(Accent.R, Accent.G, Accent.B, 0.5f+0.4f*pulse), bx + 12.f, by + 10.f, 4.f, bh - 20.f);
		DrawText(FString::Printf(TEXT("TUTORIAL  %d/6"), TStep + 1), Accent, bx + 26.f, by + 9.f, Font, CAP);
		DrawWrapped(PC->TutorialPrompt(), Prim, bx + 26.f, by + 28.f, bw - 230.f, SUB);
		Button(bx + bw - 98.f, by + bh - 32.f, 86.f, 26.f, TEXT("Skip"), FString(), Btn, Red, FName("tut_skip"), true, FString());
		if (PC->TutorialWantsNext()) Button(bx + bw - 196.f, by + bh - 32.f, 90.f, 26.f, TEXT("Next"), FString(), Accent, AccTxt, FName("tut_next"), true, FString());
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
		const float tw = 300.f, tx = FMath::Min(mx + 16.f, SW - tw - 8.f), ty = my + 18.f;
		Card(tx, ty, tw, 50.f, CardHi);
		DrawWrapped(FrameTip, Prim, tx + 12.f, ty + 9.f, tw - 24.f, CAP);
	}
}
