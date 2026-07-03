// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MarsSimTypes.h"
#include "MarsGreenhousePlayerController.generated.h"

class UGreenhouseSimSubsystem;
class UUserWidget;
class UEventPopupWidget;
class AGreenhouseCamera;
class USoundBase;

UCLASS()
class AMarsGreenhousePlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMarsGreenhousePlayerController();

	UPROPERTY(BlueprintReadOnly, Category="Sim")
	int32 SelectedBed = 0;

	bool HasCustomDashboard() const { return DashboardWidget != nullptr; }
	bool HasCustomEventUI() const;

	FString GetCameraLabel() const;

	// --- camera bank access (HUD monitor; cameras switch by CLICK only now) ---
	int32 NumCameras() const { return Cameras.Num(); }
	AGreenhouseCamera* GetCameraAt(int32 Index) const { return Cameras.IsValidIndex(Index) ? Cameras[Index] : nullptr; }
	int32 CurrentCameraIndex() const { return CamIndex; }
	void SelectCamera(int32 Index);

	// --- single entry point for EVERY clickable HUD element (see NotifyHitBoxClick) ---
	void HandleHudAction(FName Action);

	// Called by a world bed actor when the player clicks it (Sage-room interaction).
	void SelectBedFromWorld(int32 BedIndex);

	// --- guided first-sol tutorial (skippable) ---
	bool    TutorialActive() const   { return !bTutorialDone; }
	int32   TutorialStepIndex() const{ return TutorialStep; }
	FString TutorialPrompt() const;                 // text for the current step
	bool    TutorialWantsNext() const{ return TutorialStep <= 1; } // steps advanced by a Next button
	void    TutorialNext();                         // advance an intro step
	void    TutorialSkip()           { bTutorialDone = true; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	UPROPERTY() TObjectPtr<UUserWidget> DashboardWidget;

	// cameras
	UPROPERTY() TArray<TObjectPtr<AGreenhouseCamera>> Cameras;
	int32 CamIndex = 0;
	int32 PendingCam = 0;
	FTimerHandle CamFadeTimer;
	FTimerHandle DayFadeTimer;
	FRotator CamBaseRot = FRotator::ZeroRotator;
	FRotator CamPanOffset = FRotator::ZeroRotator;

	// tutorial
	int32 TutorialStep  = 0;
	bool  bTutorialDone = false;
	void  TutorialTryAdvance(FName Action);

	// cached audio
	UPROPERTY() TObjectPtr<USoundBase> SfxClick;
	UPROPERTY() TObjectPtr<USoundBase> SfxConfirm;
	UPROPERTY() TObjectPtr<USoundBase> SfxAlert;
	UPROPERTY() TObjectPtr<USoundBase> SfxTransition;

	UGreenhouseSimSubsystem* GetSim() const;
	void PlaySfx(USoundBase* Sound) const;
	void SwitchCamera(int32 Index);
	void FinishCameraSwitch();
	void DayFadeDip();
	void FinishDayFade();

	UFUNCTION() void HandleEventTriggered(FEventCard Card);
	UFUNCTION() void HandleNewSol(int32 Sol);
	UFUNCTION() void HandleHarvest(int32 BedIndex);
	UFUNCTION() void HandleFail(EResource Cause);
	UFUNCTION() void HandleWin();

	// keyboard accelerators (optional; the HUD is fully clickable). Camera keys removed.
	void KeyPause();
	void KeyAdvanceDay();
	void KeyRestart();
	void KeyNextBed();
	void KeyWash();
	void KeyPlantPotato();
	void KeyPlantLettuce();
	void KeyPlantAlgae();
	void KeyPlantLegume();
	void KeyOrderNitrogen();
	void KeyMineIce();
	void KeyElectrolyze();
	void KeyHarvestN2();
	void KeyCycleLight();
	void KeyCycleWater();
	void KeyCycleHeat();
	void KeyCycleNutrient();
	void KeyChoiceA();
	void KeyChoiceB();
};
