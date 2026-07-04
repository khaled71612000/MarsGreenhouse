// Copyright Epic Games, Inc. All Rights Reserved.
// "Bustan" HUD — concept-art palette, unified top pill, deliberate sizing hierarchy.
// Variable-text panels (tutorial, objective, tooltip) AUTO-SIZE to their content -> no overflow.
// Grow-light shown as "Red + Blue" / "White". One master text scale (FS).

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
	UFont* Font = CustomFont ? CustomFont : (GEngine ? GEngine->GetLargeFont() : nullptr);
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

	// ============================================================================
	//  MASTER TEXT SIZE.  Bump FS to scale the WHOLE HUD at once.
	// ============================================================================
	const float FS  = 1.3f;
	const float CAP = 1.02f*FS;
	const float SUB = 1.20f*FS;
	const float LBL = 1.40f*FS;
	const float VAL = 1.85f*FS;
	const float BIG = 2.45f*FS;

	// ---- concept-art palette: dark slate panels, teal-blue borders, vivid resource hues ----
	const FLinearColor Prim  (0.95f, 0.97f, 1.00f);
	const FLinearColor Sec   (0.73f, 0.79f, 0.88f);
	const FLinearColor Muted (0.54f, 0.59f, 0.69f);
	const FLinearColor Accent(0.40f, 0.72f, 1.00f);
	const FLinearColor AccTxt(0.03f, 0.07f, 0.12f);
	const FLinearColor O2C   (0.36f, 0.66f, 0.99f);
	const FLinearColor H2OC  (0.30f, 0.80f, 0.93f);
	const FLinearColor FoodC (0.47f, 0.86f, 0.38f);
	const FLinearColor PwrC  (0.99f, 0.82f, 0.28f);
	const FLinearColor Red   (0.98f, 0.44f, 0.44f);
	const FLinearColor Green (0.44f, 0.88f, 0.50f);
	const FLinearColor Warn  (0.98f, 0.62f, 0.28f);
	const FLinearColor PurpleC(0.66f, 0.46f, 1.00f);
	const FLinearColor WhiteC (0.96f, 0.97f, 1.00f);
	const FLinearColor CardBg(0.090f, 0.108f, 0.144f, 0.860f);
	const FLinearColor CardHi(0.130f, 0.156f, 0.205f, 0.900f);
	const FLinearColor Btn   (0.175f, 0.208f, 0.270f, 0.96f);
	const FLinearColor ChoiceA(0.15f, 0.32f, 0.22f, 0.99f);
	const FLinearColor ChoiceB(0.34f, 0.25f, 0.13f, 0.99f);
	const FLinearColor IconBg(0.075f, 0.090f, 0.120f, 1.f);
	const FLinearColor Border(0.42f, 0.66f, 0.86f, 0.32f);
	const FLinearColor BarBg (1.f, 1.f, 1.f, 0.09f);
	const FLinearColor Divide(1.f, 1.f, 1.f, 0.09f);

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
	auto ResIcon = [&](int32 i, float bcx, float bcy, float brad, const FLinearColor& col)
	{
		if (ResTex[i]) { DrawTexture(ResTex[i], bcx-brad, bcy-brad, brad*2.f, brad*2.f, 0.f,0.f,1.f,1.f, FLinearColor::White); return; }
		Circle(bcx, bcy, brad, col);
		Circle(bcx, bcy, brad*0.60f, FLinearColor(col.R*0.20f, col.G*0.20f, col.B*0.20f, 1.f));
		Circle(bcx, bcy, brad*0.30f, col);
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
		const int32 cells = 10; const float gap = 3.f;
		const float cw = (w - (cells-1)*gap) / cells;
		const int32 filled = FMath::RoundToInt(FMath::Clamp(pct,0.f,1.f) * cells);
		for (int32 i = 0; i < cells; ++i)
			RoundRect(x + i*(cw+gap), y, cw, h, 2.f, i < filled ? col : BarBg);
	};

	FName FrameHover; FString FrameTip;
	auto Button = [&](float x, float y, float w, float h, const FString& label, const FString& sub, const FLinearColor& fill, const FLinearColor& txt, FName id, bool enabled, const FString& tip, UTexture2D* icon = nullptr)
	{
		const bool hov = enabled && MouseIn(x, y, w, h);
		const FLinearColor bg = !enabled ? FLinearColor(0.11f,0.13f,0.17f,0.9f) : (hov ? Brighten(fill) : fill);
		if (ButtonTex) NineSlice(ButtonTex, x, y, w, h, Cfg->ButtonMargin, bg);
		else { if (hov) RoundRect(x-2.f,y-2.f,w+4.f,h+4.f,9.f,Accent); RoundRect(x, y, w, h, 7.f, bg); }
		float tx = x + 15.f;
		if (icon) { const float is = FMath::Min(h - 8.f, 46.f); DrawTexture(icon, x + 12.f, y + (h - is)*0.5f, is, is, 0.f,0.f,1.f,1.f, enabled ? FLinearColor::White : FLinearColor(1,1,1,0.4f)); tx = x + 12.f + is + 9.f; }
		DrawText(label, enabled ? txt : Muted, tx, y + (sub.IsEmpty() ? h*0.5f - 13.f : 7.f), Font, LBL);
		if (!sub.IsEmpty()) DrawText(sub, enabled ? FLinearColor(txt.R*0.82f,txt.G*0.82f,txt.B*0.82f,0.94f) : Muted, tx, y + h - 24.f, Font, CAP);
		if (enabled) { AddHitBox(FVector2D(x,y), FVector2D(w,h), id, true); if (hov) { FrameHover = id; FrameTip = tip; } }
	};
	// shared wrap heuristic (tuned for the loaded runtime font)
	const float kCharW = 11.0f;
	auto WrapCount = [&](const FString& Text, float maxW, float scale) -> int32
	{
		TArray<FString> Words; Text.ParseIntoArray(Words, TEXT(" "));
		const int32 maxChars = FMath::Max(8, (int32)(maxW / (kCharW*scale)));
		FString Ln; int32 n = 0;
		for (const FString& Wd : Words) { if (Ln.Len()+Wd.Len()+1 > maxChars) { ++n; Ln = Wd; } else { Ln = Ln.IsEmpty()?Wd:Ln+TEXT(" ")+Wd; } }
		if (!Ln.IsEmpty()) ++n;
		return FMath::Max(1, n);
	};
	auto DrawWrapped = [&](const FString& Text, const FLinearColor& Col, float x, float y, float maxW, float scale) -> float
	{
		TArray<FString> Words; Text.ParseIntoArray(Words, TEXT(" "));
		const int32 maxChars = FMath::Max(8, (int32)(maxW / (kCharW*scale)));
		const float lineH = 27.f * scale;
		FString Ln; float ly = y;
		for (const FString& Wd : Words)
		{
			if (Ln.Len() + Wd.Len() + 1 > maxChars) { DrawText(Ln, Col, x, ly, Font, scale); ly += lineH; Ln = Wd; }
			else { Ln = Ln.IsEmpty() ? Wd : Ln + TEXT(" ") + Wd; }
		}
		if (!Ln.IsEmpty()) { DrawText(Ln, Col, x, ly, Font, scale); ly += lineH; }
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
		// Per-room interactables: only show clickables that belong to the ACTIVE camera's room.
		// Tag a planter/LED (Details > Actor > Tags) with the room name; set that same string as the
		// camera's RoomLabel. Untagged actors show on every camera; a camera with no RoomLabel shows all.
		FName ActiveRoom = NAME_None;
		if (PC && PC->NumCameras() > 0)
			if (AGreenhouseCamera* AC = PC->GetCameraAt(PC->CurrentCameraIndex()))
				if (!AC->RoomLabel.IsEmpty()) ActiveRoom = FName(*AC->RoomLabel);
		auto InRoom = [&](const AActor* A){ return ActiveRoom.IsNone() || A->Tags.Num() == 0 || A->ActorHasTag(ActiveRoom); };

		auto WorldTag = [&](const FVector& Loc, const FString& Txt, const FLinearColor& col, bool bold)
		{
			const FVector Scr = Project(Loc);
			if (Scr.Z <= 0.f) return FVector2D(-1.f,-1.f);
			const float tw = 20.f + Txt.Len() * 11.0f;
			const float tx = Scr.X - tw*0.5f, ty = Scr.Y;
			Card(tx, ty, tw, 32.f, CardBg);
			if (bold) Outline(tx, ty, tw, 32.f, col, 1.f);
			DrawText(Txt, bold ? col : Prim, tx + 11.f, ty + 6.f, Font, CAP);
			return FVector2D(Scr.X, Scr.Y);
		};

		TArray<AActor*> LabelActors;
		UGameplayStatics::GetAllActorsOfClass(this, AGreenhouseLabel::StaticClass(), LabelActors);
		for (AActor* A : LabelActors)
			if (AGreenhouseLabel* Lab = Cast<AGreenhouseLabel>(A))
				if (InRoom(Lab))
					WorldTag(Lab->GetActorLocation() + FVector(0,0,Lab->HeightOffset), Lab->LabelText, Accent, false);

		TArray<AActor*> BedActors;
		UGameplayStatics::GetAllActorsOfClass(this, AGreenhouseBed::StaticClass(), BedActors);
		for (AActor* A : BedActors)
		{
			AGreenhouseBed* Bed = Cast<AGreenhouseBed>(A);
			if (!Bed || !S->Beds.IsValidIndex(Bed->BedIndex)) continue;
			if (!InRoom(Bed)) continue;
			const int32 bi = Bed->BedIndex;
			// project the 8 world-bounds corners -> a tight screen box that hugs the mesh at any angle
			FVector BO, BE; Bed->GetActorBounds(false, BO, BE);
			float minx=1e9f, miny=1e9f, maxx=-1e9f, maxy=-1e9f; bool anyFront=false;
			for (int32 c = 0; c < 8; ++c)
			{
				const FVector corner = BO + FVector((c&1)?BE.X:-BE.X, (c&2)?BE.Y:-BE.Y, (c&4)?BE.Z:-BE.Z);
				const FVector cs = Project(corner);
				if (cs.Z > 0.f) { anyFront=true; minx=FMath::Min(minx,cs.X); miny=FMath::Min(miny,cs.Y); maxx=FMath::Max(maxx,cs.X); maxy=FMath::Max(maxy,cs.Y); }
			}
			if (!anyFront) continue;
			minx=FMath::Clamp(minx,-40.f,SW+40.f); maxx=FMath::Clamp(maxx,-40.f,SW+40.f);
			miny=FMath::Clamp(miny,-40.f,SH+40.f); maxy=FMath::Clamp(maxy,-40.f,SH+40.f);
			const float bxw = FMath::Max(26.f, maxx-minx), byh = FMath::Max(22.f, maxy-miny);
			const float px = (minx+maxx)*0.5f, py = (miny+maxy)*0.5f;
			const FPlantedBed& B = S->Beds[bi];
			const bool ready = S->IsBedReady(bi);
			const bool sel   = (bi == Sel);
			const bool hov   = MouseIn(minx, miny, bxw, byh);
			const FLinearColor hc = BedStateColor(B, bi);
			const FLinearColor oc = sel ? Accent : (ready ? FLinearColor(hc.R,hc.G,hc.B, 0.5f+0.5f*pulse) : (hov ? Prim : FLinearColor(hc.R,hc.G,hc.B,0.55f)));
			Outline(minx, miny, bxw, byh, oc, (sel||hov)?2.5f:1.f);
			Circle(px, miny-8.f, 5.f, hc);
			DrawText(FString::Printf(TEXT("Planter %d"), bi), sel ? Accent : Sec, minx, maxy+4.f, Font, CAP);
			AddHitBox(FVector2D(minx, miny), FVector2D(bxw, byh), FName(*FString::Printf(TEXT("bed%d"), bi)), true);

			if (sel && bLive)
			{
				const float mw = 200.f, ax = px - mw*0.5f;
				const float ay = FMath::Min(miny - 92.f, SH - 252.f);
				if (!B.bOccupied)
				{
					const float hw = (mw-6.f)*0.5f;
					Button(ax,        ay, hw, 38.f, TEXT("Potato"),  FString(), Btn, Prim, FName("plant_potato"),  true, TEXT("Potato - slow, high food, likes white light."));
					Button(ax+hw+6.f, ay, hw, 38.f, TEXT("Lettuce"), FString(), Btn, Prim, FName("plant_lettuce"), true, TEXT("Lettuce - fast, good oxygen, likes red + blue light."));
				}
				else if (B.bSpoiled) Button(ax, ay, mw, 38.f, TEXT("Clear planter"), FString(), Btn, Red, FName("act_harvest"), true, TEXT("Remove the spoiled crop."));
				else if (ready)      Button(ax, ay, mw, 38.f, TEXT("Harvest now"),   FString(), Accent, AccTxt, FName("act_harvest"), true, TEXT("Collect before it spoils."));
				else
				{
					const float hw = (mw-6.f)*0.5f;
					Button(ax,        ay, hw, 38.f, TEXT("Water"),   FString(), Btn, Prim, FName("act_water"),   true, TEXT("Restore health."));
					Button(ax+hw+6.f, ay, hw, 38.f, TEXT("Harvest"), FString(), Btn, Prim, FName("act_harvest"), true, TEXT("Harvest early (partial)."));
				}
			}
		}

#if 0 // grow-light WORLD highlighters disabled - controlled from the HUD grow-light toggle only
		TArray<AActor*> LedActors;
		UGameplayStatics::GetAllActorsOfClass(this, AGreenhouseLED::StaticClass(), LedActors);
		for (AActor* A : LedActors)
		{
			if (!InRoom(A)) continue;
			FVector LO, LE; A->GetActorBounds(false, LO, LE);
			const FVector2D P = WorldTag(LO + FVector(0,0,LE.Z + 10.f), TEXT("Grow Lights"), S->LedColor==ELedColor::White?WhiteC:PurpleC, true);
			if (P.X < 0.f) continue;
			const bool bNear = FMath::Abs(P.X-mx) < 170.f && FMath::Abs(P.Y-my) < 84.f;
			if (bNear && bLive)
			{
				const float bw = 152.f, ax = P.X - (bw*2+8.f)*0.5f, ay = P.Y - 48.f;
				Button(ax,        ay, bw, 36.f, TEXT("Red + Blue"), FString(), S->LedColor==ELedColor::Purple?PurpleC:Btn, S->LedColor==ELedColor::Purple?AccTxt:Prim, FName("led_purple"), true, TEXT("Red + Blue: more oxygen, steady growth (lettuce)."));
				Button(ax+bw+8.f, ay, bw, 36.f, TEXT("White"),      FString(), S->LedColor==ELedColor::White ?WhiteC :Btn, S->LedColor==ELedColor::White?AccTxt:Prim,  FName("led_white"),  true, TEXT("White: faster growth, less oxygen (potato)."));
				break;
			}
		}
#endif

	}

	if (!bCustomDash)
	{
		// ================= TOP RESOURCE PILL (unified) =================
		{
			const TCHAR* RN[4] = { TEXT("OXYGEN"), TEXT("WATER"), TEXT("FOOD"), TEXT("POWER") };
			const FLinearColor RC[4] = { O2C, H2OC, FoodC, PwrC };
			const float cellW = 252.f, pillH = 66.f, px0 = 14.f, py = 10.f;
			const float pillW = cellW * 4.f;
			Card(px0, py, pillW, pillH, CardBg);
			for (int32 i = 0; i < 4; ++i)
			{
				const float cx = px0 + i*cellW;
				const float Val = Meters[i];
				DispVal[i] = FMath::FInterpTo(DispVal[i], Val, dt, 8.f);
				if (FMath::Abs(Val - LastVal[i]) >= 1.f) { LastDelta[i] = Val - LastVal[i]; Floaters.Add({ FString::Printf(TEXT("%+d"), FMath::RoundToInt(LastDelta[i])), LastDelta[i]>0?Green:Red, cx + cellW*0.5f, py + pillH + 4.f, 1.3f }); }
				LastVal[i] = Val;
				const bool low = Val <= 15.f;
				ResIcon(i, cx + 36.f, py + pillH*0.5f, 20.f, RC[i]);
				DrawText(RN[i], Sec, cx + 62.f, py + 11.f, Font, CAP);
				DrawText(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(DispVal[i])), low ? Red : Prim, cx + cellW - 78.f, py + 11.f, Font, LBL);
				SegMeter(cx + 62.f, py + 42.f, cellW - 86.f, 12.f, DispVal[i]*0.01f, low ? Red : RC[i]);
				if (i < 3) DrawRect(Divide, cx + cellW - 1.f, py + 14.f, 1.f, pillH - 28.f);
			}
			const float pw = 166.f, pxp = SW - pw - 14.f;
			Card(pxp, py, pw, pillH, CardBg);
			DrawText(TEXT("SOL"), Muted, pxp + 18.f, py + 11.f, Font, CAP);
			DrawText(FString::Printf(TEXT("%02d"), S->Sol), Accent, pxp + 18.f, py + 27.f, Font, VAL);
			DrawText(FString::Printf(TEXT("/ %d"), Cfg->TotalSols), Sec, pxp + 84.f, py + 39.f, Font, CAP);
			DrawText(bLive ? TEXT("DAY") : TEXT("..."), Sec, pxp + pw - 54.f, py + 13.f, Font, CAP);
		}

		{
			const float sbw = 700.f, sx = SW*0.5f - sbw*0.5f, sy = 84.f;
			if (S->DustLevel > 0.f) { Card(sx, sy, sbw, 40.f, FLinearColor(0.24f,0.13f,0.10f,0.99f)); DrawText(FString::Printf(TEXT("DUST STORM   solar -%d%% this sol"), FMath::RoundToInt(S->DustLevel*100.f)), Warn, sx + 20.f, sy + 9.f, Font, SUB); }
			else if (S->NextDust > 0.f) { Card(sx, sy, sbw, 36.f, CardBg); DrawText(FString::Printf(TEXT("Storm forecast next sol (solar -%d%%) - bank power"), FMath::RoundToInt(S->NextDust*100.f)), Warn, sx + 20.f, sy + 7.f, Font, SUB); }
		}

		// ================= RIGHT: CREW STATUS =================
		{
			const float cw = 410.f, cx = SW - cw - 14.f, cyTop = 88.f;
			const int32 NCrew = Cfg->Crew.Num();
			const float rowH = 58.f, ph = 42.f + NCrew * rowH;
			Card(cx, cyTop, cw, ph, CardBg);
			DrawText(TEXT("CREW STATUS"), Accent, cx + 18.f, cyTop + 13.f, Font, SUB);
			for (int32 i = 0; i < NCrew; ++i)
			{
				const float ry = cyTop + 42.f + i * rowH;
				UTexture2D* Portrait = Cfg->CrewPortraits.IsValidIndex(i) ? L(Cfg->CrewPortraits[i]) : nullptr;
				if (Portrait) DrawTexture(Portrait, cx + 14.f, ry + 6.f, 42.f, 42.f, 0.f,0.f,1.f,1.f);
				else { RoundRect(cx + 14.f, ry + 6.f, 42.f, 42.f, 10.f, FLinearColor(0.17f,0.21f,0.28f,1.f)); DrawText(Cfg->Crew[i].Name.ToString().Left(1), Accent, cx + 27.f, ry + 15.f, Font, SUB); }
				DrawText(Cfg->Crew[i].Name.ToString(), Prim, cx + 66.f, ry + 6.f, Font, CAP);
				DrawText(Cfg->Crew[i].Role.ToString(), Muted, cx + 66.f, ry + 30.f, Font, CAP);
				const FString St = S->GetCrewStatus(i);
				const FLinearColor sc = (St == TEXT("OK")) ? Green : (St == TEXT("Critical") ? Red : Warn);
				DrawText(St, sc, cx + cw - 56.f, ry + 16.f, Font, CAP);
			}
		}

		// ================= LEFT: CAMERAS (show ALL cameras) =================
		if (PC && PC->NumCameras() > 0)
		{
			const int32 NC = PC->NumCameras();
			const float maxRowW = SW * 0.46f;                 // keep the row clear of the centered banner
			float tw = 138.f, th = 78.f;
			const float need = NC*tw + (NC-1)*8.f;
			if (need > maxRowW) { tw = FMath::Max(82.f, (maxRowW - (NC-1)*8.f)/NC); th = tw*0.565f; }
			const float y0 = 88.f;
			for (int32 i = 0; i < NC; ++i)
			{
				AGreenhouseCamera* Cam = PC->GetCameraAt(i);
				const float x = 14.f + i*(tw+8.f);
				const bool active = (i == PC->CurrentCameraIndex());
				DrawRect(FLinearColor(0.02f,0.03f,0.05f,1.f), x, y0, tw, th);
				if (Cam && Cam->GetPreviewRT()) DrawTexture(Cam->GetPreviewRT(), x, y0, tw, th, 0.f,0.f,1.f,1.f);
				Outline(x, y0, tw, th, active ? Accent : Border, active ? 2.f : 1.f);
				DrawText(Cam ? Cam->RoomLabel : FString::Printf(TEXT("Cam %d"),i), active ? Accent : Prim, x + 8.f, y0 + th - 22.f, Font, CAP);
				AddHitBox(FVector2D(x,y0), FVector2D(tw,th), FName(*FString::Printf(TEXT("cam%d"),i)), true);
			}
		}

		// ================= LEFT-BOTTOM: OBJECTIVE (auto-height, bottom-anchored) + LOG above =================
		{
			const float lw = 480.f, lx = 14.f;
			const FString Obj = S->CurrentObjective();
			const float ocw = lw - 36.f;
			const int32 oLines = WrapCount(Obj, ocw, CAP);
			const float oh = 50.f + oLines*(32.f*CAP);   // header + generous per-line, always > text
			const float oy = SH - 96.f - oh;
			Card(lx, oy, lw, oh, CardBg);
			DrawRect(Accent, lx, oy, 4.f, oh);
			DrawText(TEXT("OBJECTIVE"), Accent, lx + 18.f, oy + 13.f, Font, SUB);
			DrawWrapped(Obj, Prim, lx + 18.f, oy + 44.f, ocw, CAP);

			const float lh = 200.f, ly = oy - 8.f - lh;
			Card(lx, ly, lw, lh, CardBg);
			DrawText(TEXT("MISSION LOG"), Accent, lx + 18.f, ly + 13.f, Font, SUB);
			const int32 N = S->MissionLog.Num(), Show = FMath::Min(5, N);
			for (int32 i = 0; i < Show; ++i)
			{
				FString Ln = S->MissionLog[N - Show + i];
				if (Ln.Len() > 54) Ln = Ln.Left(53) + TEXT("...");
				const bool last = (i == Show-1);
				Circle(lx + 24.f, ly + 52.f + i*30.f, 3.f, last ? Accent : Muted);
				DrawText(Ln, last ? Prim : Sec, lx + 34.f, ly + 45.f + i*30.f, Font, CAP);
			}
		}

		// ================= BOTTOM-CENTER: GROW-LIGHT + PLANTERS + STAGE =================
		{
			const float py0 = SH - 264.f;
			const float gw = 348.f, gx = SW*0.5f - gw*0.5f;
			Card(gx, py0, gw, 42.f, CardBg);
			const float seg = (gw-6.f)/2.f;
			Button(gx+3.f,     py0+3.f, seg-2.f, 36.f, TEXT("Red + Blue"), FString(), S->LedColor==ELedColor::Purple?PurpleC:Btn, S->LedColor==ELedColor::Purple?AccTxt:Sec, FName("led_purple"), true, TEXT("Red + Blue: more oxygen, steady growth (lettuce)."));
			Button(gx+3.f+seg, py0+3.f, seg-2.f, 36.f, TEXT("White"),      FString(), S->LedColor==ELedColor::White ?WhiteC :Btn, S->LedColor==ELedColor::White?AccTxt:Sec,  FName("led_white"), true, TEXT("White: faster growth, less oxygen (potato)."));
			DrawText(TEXT("Grow-light"), Muted, gx, py0 + 46.f, Font, CAP);

			const float cy = py0 + 74.f; const int32 NB = S->Beds.Num();
			// one neat centered row that auto-shrinks the chips so ALL planters fit on screen
			const float cg = 8.f, maxRowW = SW * 0.66f;
			float chw = 72.f;
			if (NB*chw + (NB-1)*cg > maxRowW) chw = FMath::Max(38.f, (maxRowW - (NB-1)*cg)/NB);
			const float tot = NB*chw + (NB-1)*cg, cx0 = SW*0.5f - tot*0.5f;
			for (int32 i = 0; i < NB; ++i)
			{
				const float x = cx0 + i*(chw+cg); const bool sel = (i==Sel);
				Card(x, cy, chw, 38.f, sel ? CardHi : CardBg);
				if (sel) Outline(x, cy, chw, 38.f, Accent, 2.f);
				DrawText(FString::Printf(TEXT("%d"), i), sel ? Accent : Sec, x + 10.f, cy + 8.f, Font, LBL);
				Circle(x + chw - 12.f, cy + 19.f, 4.f, BedStateColor(S->Beds[i], i));
				AddHitBox(FVector2D(x, cy), FVector2D(chw, 38.f), FName(*FString::Printf(TEXT("bed%d"), i)), true);
			}

			if (S->Beds.IsValidIndex(Sel))
			{
				const FPlantedBed& B = S->Beds[Sel];
				const float pw = 464.f, sx = SW*0.5f - pw*0.5f, sy = cy + 46.f;
				Card(sx, sy, pw, 48.f, CardBg);
				if (!B.bOccupied) DrawText(FString::Printf(TEXT("Planter %d - empty, plant a crop"), Sel), Muted, sx + 18.f, sy + 15.f, Font, SUB);
				else
				{
					const FCropStats& C = Cfg->GetCrop(B.Crop);
					const int32 stg = FMath::Clamp(CropStageIndex(C.StageEnds, B.Growth), 0, FMath::Max(0, C.StageNames.Num()-1));
					const FString Stage = B.bSpoiled ? FString::Printf(TEXT("Spoiled (%s)"), *C.SpoilName.ToString()) : (S->IsBedReady(Sel) ? FString::Printf(TEXT("READY  %d sol left"), S->SolsLeftToHarvest(Sel)) : (C.StageNames.IsValidIndex(stg) ? C.StageNames[stg].ToString() : FString(CropName(B.Crop))));
					const FLinearColor pc = B.bSpoiled ? Red : (S->IsBedReady(Sel) ? Green : Warn);
					DrawText(FString::Printf(TEXT("%s  -  %s"), CropName(B.Crop), *Stage), pc, sx + 18.f, sy + 6.f, Font, SUB);
					Bar(sx + 18.f, sy + 32.f, pw - 36.f, 8.f, B.Growth, pc);
				}
			}
		}
	}

	// ================= BOTTOM ACTION DOCK =================
	if (!bEnded && !bCustomDash)
	{
		const float M = 14.f, availW = SW - 2.f*M, barH = 70.f, barY = SH - barH - 10.f;
		Card(M - 4.f, barY - 6.f, availW + 8.f, barH + 2.f, CardBg);
		const bool bOcc = S->Beds.IsValidIndex(Sel) && S->Beds[Sel].bOccupied;
		const bool bReady = S->IsBedReady(Sel);
		const bool bWater = bOcc && !S->Beds[Sel].bSpoiled && S->Water >= Cfg->WaterPlantWaterCost && S->Power >= Cfg->WaterPlantPowerCost && bLive;
		const bool bHarv = bOcc && bLive;
		const bool bIce = bLive && S->Power >= Cfg->MineIcePowerCost;
		const bool bEle = bLive && S->Water >= Cfg->ElectrolyzeWaterCost && S->Power >= Cfg->ElectrolyzePowerCost;
		const FLinearColor HF = bReady ? Accent : Btn;
		const FLinearColor HT = bReady ? AccTxt : Prim;
		const float endW = 236.f, g = 8.f, dockW = availW - endW - g;
		const int32 NBD = 5; const float bw = (dockW - (NBD-1)*g)/NBD, bh = 56.f, ry = barY + 5.f;
		UTexture2D* IcPlant=L(Cfg->IconPlant), *IcWat=L(Cfg->IconWaterAction), *IcHarv=L(Cfg->IconHarvest), *IcIce=L(Cfg->IconMineIce), *IcEle=L(Cfg->IconElectrolyze), *IcEnd=L(Cfg->IconEndDay);
		auto AB = [&](int32 i, const FString& lbl, const FString& sub, const FLinearColor& c, const FLinearColor& tc, FName id, bool en, const FString& tip, UTexture2D* ic){ Button(M + i*(bw+g), ry, bw, bh, lbl, sub, c, tc, id, en, tip, ic); };
		AB(0, TEXT("Plant"), TEXT("potato / lettuce"), Btn, Prim, FName("plant_potato"), bLive && !bOcc, TEXT("Plant a crop in the selected planter."), IcPlant);
		AB(1, TEXT("Water"), FString::Printf(TEXT("-%d H2O"), FMath::RoundToInt(Cfg->WaterPlantWaterCost)), Btn, Prim, FName("act_water"), bWater, TEXT("Water the selected planter. Neglected plants die."), IcWat);
		AB(2, bReady ? TEXT("Harvest !") : TEXT("Harvest"), TEXT("[E]"), HF, HT, FName("act_harvest"), bHarv, TEXT("Collect. Early = partial; too late = spoiled."), IcHarv);
		AB(3, TEXT("Mine ice"), FString::Printf(TEXT("+%d -%d Pwr"), FMath::RoundToInt(Cfg->MineIceWater), FMath::RoundToInt(Cfg->MineIcePowerCost)), Btn, Prim, FName("act_ice"), bIce, TEXT("Water from ice. Costs power."), IcIce);
		AB(4, TEXT("Electrolyze"), FString::Printf(TEXT("+%d O2"), FMath::RoundToInt(Cfg->ElectrolyzeOxygen)), Btn, Prim, FName("act_electro"), bEle, TEXT("Water into oxygen. Costs water + power."), IcEle);
		Button(SW - M - endW, ry, endW, bh, TEXT("END DAY"), TEXT("[Enter]"), Accent, AccTxt, FName("advance"), bLive, TEXT("Resolve the sol: crew eats, crops grow, weather turns."), IcEnd);
	}

	// ================= EVENT CARD (auto-height) =================
	if (S->bEventActive && !bCustomEvent)
	{
		const FString Sit = S->CurrentEvent.Situation.ToString();
		const float w = 884.f, ex = SW*0.5f - w*0.5f;
		const int32 sLines = WrapCount(Sit, w - 60.f, LBL);
		const bool single = S->CurrentEvent.bSingleChoice;
		const float choicesH = single ? 72.f : 122.f;
		const float h = 100.f + sLines*(32.f*LBL) + choicesH;
		const float ey = SH*0.40f - h*0.5f;
		Card(ex, ey, w, h, CardHi);
		DrawRect(Accent, ex + 14.f, ey + 16.f, 4.f, 28.f);
		DrawText(single ? TEXT("STORY") : TEXT("DECISION"), Accent, ex + 30.f, ey + 18.f, Font, SUB);
		DrawText(S->CurrentEvent.Speaker.ToString(), Prim, ex + 30.f, ey + 46.f, Font, VAL);
		DrawWrapped(Sit, Sec, ex + 30.f, ey + 92.f, w - 60.f, LBL);
		if (single)
			Button(ex + 30.f, ey + h - 70.f, w - 60.f, 52.f, S->CurrentEvent.ChoiceA.Label.ToString(), FString(), Accent, AccTxt, FName("evt_a"), true, FString());
		else
		{
			const float ax = ex + 30.f, aw = w - 60.f;
			Button(ax, ey + h - 114.f, aw, 50.f, FString::Printf(TEXT("A:  %s"), *S->CurrentEvent.ChoiceA.Label.ToString()), EffectArrows(S->CurrentEvent.ChoiceA.Effect), ChoiceA, Prim, FName("evt_a"), true, FString());
			DrawRect(Green, ax + 2.f, ey + h - 110.f, 5.f, 42.f);
			Button(ax, ey + h - 56.f, aw, 50.f, FString::Printf(TEXT("B:  %s"), *S->CurrentEvent.ChoiceB.Label.ToString()), EffectArrows(S->CurrentEvent.ChoiceB.Effect), ChoiceB, Prim, FName("evt_b"), true, FString());
			DrawRect(Warn, ax + 2.f, ey + h - 52.f, 5.f, 42.f);
		}
	}

	// ================= WIN / LOSE =================
	if (bEnded)
	{
		const bool bWon = S->State == EGameState::Won;
		DrawRect(FLinearColor(0.03f,0.04f,0.06f,0.92f), 0.f, 0.f, SW, SH);
		const float cw = 500.f, cx = SW*0.5f - cw*0.5f, cy = SH*0.5f - 170.f;
		Card(cx, cy, cw, 340.f, CardHi);
		if (LogoTex) DrawTexture(LogoTex, cx + cw*0.5f - 90.f, cy + 18.f, 180.f, 56.f, 0.f,0.f,1.f,1.f);
		DrawText(bWon ? TEXT("BUSTAN ENDURES") : TEXT("THE GARDEN FELL"), bWon ? Green : Red, cx + 34.f, cy + (LogoTex?86.f:30.f), Font, BIG);
		DrawText(FString::Printf(TEXT("Sols survived:  %d / %d"), FMath::Min(S->Sol - 1, Cfg->TotalSols), Cfg->TotalSols), Prim, cx + 34.f, cy + 140.f, Font, VAL);
		DrawText(FString::Printf(TEXT("Harvests:  %d       Crops lost:  %d"), S->Harvests, S->Spoiled), Sec, cx + 34.f, cy + 178.f, Font, LBL);
		DrawText(TEXT("RANK"), Muted, cx + 34.f, cy + 220.f, Font, SUB);
		DrawText(S->ResultRank(), Accent, cx + 128.f, cy + 208.f, Font, BIG);
		Button(cx + 34.f, cy + 272.f, cw - 68.f, 52.f, TEXT("RESTART RUN"), FString(), Accent, AccTxt, FName("restart"), true, FString());
	}

	// ================= TUTORIAL (auto-height) =================
	if (PC && PC->TutorialActive() && !bEnded)
	{
		const FString Prompt = PC->TutorialPrompt();
		const float bw = 864.f, bx = SW*0.5f - bw*0.5f, wrapW = bw - 56.f;
		const int32 tLines = WrapCount(Prompt, wrapW, SUB);
		const float bh = 52.f + tLines*(32.f*SUB) + 46.f;   // header + generous text + button row
		const float by = SH*0.56f - bh*0.5f;
		Card(bx, by, bw, bh, CardHi);
		DrawRect(FLinearColor(Accent.R, Accent.G, Accent.B, 0.5f+0.4f*pulse), bx + 14.f, by + 12.f, 4.f, bh - 24.f);
		DrawText(FString::Printf(TEXT("TUTORIAL  %d/6"), TStep + 1), Accent, bx + 28.f, by + 12.f, Font, CAP);
		DrawWrapped(Prompt, Prim, bx + 28.f, by + 44.f, wrapW, SUB);
		Button(bx + bw - 108.f, by + bh - 40.f, 94.f, 32.f, TEXT("Skip"), FString(), Btn, Red, FName("tut_skip"), true, FString());
		if (PC->TutorialWantsNext()) Button(bx + bw - 214.f, by + bh - 40.f, 98.f, 32.f, TEXT("Next"), FString(), Accent, AccTxt, FName("tut_next"), true, FString());
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

	// ================= TOOLTIP (auto-height) =================
	if (FrameHover == HoverBox) HoverTime += dt; else { HoverBox = FrameHover; HoverTime = 0.f; }
	if (HoverTime > 0.6f && !FrameTip.IsEmpty() && mx > 0.f)
	{
		const float tw = 340.f, tx = FMath::Min(mx + 16.f, SW - tw - 8.f);
		const int32 ttl = WrapCount(FrameTip, tw - 28.f, CAP);
		const float tth = 26.f + ttl*(32.f*CAP);
		const float ty = FMath::Min(my + 18.f, SH - tth - 8.f);
		Card(tx, ty, tw, tth, CardHi);
		DrawWrapped(FrameTip, Prim, tx + 14.f, ty + 11.f, tw - 28.f, CAP);
	}
}
