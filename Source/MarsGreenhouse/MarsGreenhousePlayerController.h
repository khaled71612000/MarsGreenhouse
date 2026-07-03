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

	// --- camera bank (HUD monitor; switch by click) ---
	int32 NumCameras() const { return Cameras.Num(); }
	AGreenhouseCamera* GetCameraAt(int32 Index) const { return Cameras.IsValidIndex(Index) ? Cameras[Index] : nullptr; }
	int32 CurrentCameraIndex() const { return CamIndex; }
	void SelectCamera(int32 Index);

	// --- ONE entry point for every clickable HUD / world element ---
	void HandleHudAction(FName Action);

	// Called by a world planter/LED actor when clicked.
	void SelectBedFromWorld(int32 BedIndex);

	// --- guided tutorial (skippable) ---
	bool    TutorialActive() const    { return !bTutorialDone; }
	int32   TutorialStepIndex() const { return TutorialStep; }
	FString TutorialPrompt() const;
	bool    TutorialWantsNext() const { return TutorialStep <= 1; }
	void    TutorialNext();
	void    TutorialSkip()            { bTutorialDone = true; }

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

	// keyboard accelerators (optional; the HUD + world are fully clickable)
	void KeyAdvanceDay();
	void KeyRestart();
	void KeyNextBed();
	void KeyPlantPotato();
	void KeyPlantLettuce();
	void KeyWater();
	void KeyHarvest();
	void KeyMineIce();
	void KeyElectrolyze();
	void KeyCycleLed();
	void KeyChoiceA();
	void KeyChoiceB();
};
