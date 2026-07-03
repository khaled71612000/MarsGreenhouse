// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "MarsGreenhouseHUD.generated.h"

UCLASS()
class AMarsGreenhouseHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;
	virtual void NotifyHitBoxClick(FName BoxName) override;

private:
	// Floating "+N / -N" feedback that rises and fades on any resource change.
	struct FFloatText { FString Text; FLinearColor Color; float X; float Y; float Life; };
	TArray<FFloatText> Floaters;

	float DispVal[4]   = {0,0,0,0};  // animated (tweened) meter values
	float LastVal[4]   = {0,0,0,0};  // last frame's real values (to detect changes)
	float LastDelta[4] = {0,0,0,0};  // last net change (shown as the trend on each card)
	bool  bInitVals    = false;

	FName HoverBox;                  // which button the mouse is over
	float HoverTime = 0.f;           // hover duration (for tooltips)
};
