// Copyright Epic Games, Inc. All Rights Reserved.

#include "MarsGreenhousePlayerController.h"
#include "GreenhouseSimSubsystem.h"
#include "MarsSimSettings.h"
#include "EventPopupWidget.h"
#include "GreenhouseCamera.h"
#include "Camera/CameraComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Components/InputComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"
#include "TimerManager.h"

AMarsGreenhousePlayerController::AMarsGreenhousePlayerController()
{
	bShowMouseCursor       = true;
	bEnableClickEvents     = true;  // click world objects (planters, LED)
	bEnableMouseOverEvents = true;  // hover feedback
}

UGreenhouseSimSubsystem* AMarsGreenhousePlayerController::GetSim() const
{
	return GetWorld() ? GetWorld()->GetSubsystem<UGreenhouseSimSubsystem>() : nullptr;
}

bool AMarsGreenhousePlayerController::HasCustomEventUI() const
{
	return !GetDefault<UMarsSimSettings>()->EventPopupWidgetClass.IsNull();
}

FString AMarsGreenhousePlayerController::GetCameraLabel() const
{
	if (Cameras.IsValidIndex(CamIndex) && Cameras[CamIndex])
		return FString::Printf(TEXT("%s / %s"), *Cameras[CamIndex]->RoomLabel, *Cameras[CamIndex]->CameraLabel);
	return TEXT("-");
}

void AMarsGreenhousePlayerController::PlaySfx(USoundBase* Sound) const
{
	if (Sound) UGameplayStatics::PlaySound2D(this, Sound);
}

void AMarsGreenhousePlayerController::BeginPlay()
{
	Super::BeginPlay();

	const UMarsSimSettings* Cfg = GetDefault<UMarsSimSettings>();

	if (UClass* DashClass = Cfg->DashboardWidgetClass.LoadSynchronous())
	{
		DashboardWidget = CreateWidget<UUserWidget>(this, DashClass);
		if (DashboardWidget) DashboardWidget->AddToViewport();
	}

	SfxClick      = Cfg->SfxClick.LoadSynchronous();
	SfxConfirm    = Cfg->SfxConfirm.LoadSynchronous();
	SfxAlert      = Cfg->SfxAlert.LoadSynchronous();
	SfxTransition = Cfg->SfxTransition.LoadSynchronous();
	if (USoundBase* Music = Cfg->MusicLoop.LoadSynchronous())
		UGameplayStatics::SpawnSound2D(this, Music, 0.55f);

	TArray<AActor*> Found;
	UGameplayStatics::GetAllActorsOfClass(this, AGreenhouseCamera::StaticClass(), Found);
	for (AActor* A : Found) if (auto* Cam = Cast<AGreenhouseCamera>(A)) Cameras.Add(Cam);
	if (Cameras.Num() > 0)
	{
		CamIndex = 0;
		CamBaseRot = Cameras[0]->GetActorRotation();
		SetViewTarget(Cameras[0]);
	}

	if (auto* S = GetSim())
	{
		S->OnEventTriggered.AddDynamic(this, &AMarsGreenhousePlayerController::HandleEventTriggered);
		S->OnNewSol.AddDynamic(this, &AMarsGreenhousePlayerController::HandleNewSol);
		S->OnHarvest.AddDynamic(this, &AMarsGreenhousePlayerController::HandleHarvest);
		S->OnFail.AddDynamic(this, &AMarsGreenhousePlayerController::HandleFail);
		S->OnWin.AddDynamic(this, &AMarsGreenhousePlayerController::HandleWin);
	}
}

// Slight look: cursor near a screen edge nudges the current camera a few degrees, eased.
void AMarsGreenhousePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (!Cameras.IsValidIndex(CamIndex)) return;
	AGreenhouseCamera* Cam = Cameras[CamIndex];
	if (!Cam) return;

	float mx = 0.f, my = 0.f;
	if (!GetMousePosition(mx, my)) return;
	int32 vx = 0, vy = 0;
	GetViewportSize(vx, vy);
	if (vx <= 0 || vy <= 0) return;

	const float nx = FMath::Clamp((mx / (float)vx) * 2.f - 1.f, -1.f, 1.f);
	const float ny = FMath::Clamp((my / (float)vy) * 2.f - 1.f, -1.f, 1.f);

	const FRotator Target(-ny * Cam->PanPitchLimit, nx * Cam->PanYawLimit, 0.f);
	CamPanOffset = FMath::RInterpTo(CamPanOffset, Target, DeltaTime, 6.f);
	Cam->SetActorRotation(CamBaseRot + CamPanOffset);

	// very slight cinematic zoom breathing
	if (Cam->GetCameraComponent())
		Cam->GetCameraComponent()->SetFieldOfView(Cam->CameraFov + FMath::Sin(GetWorld()->GetTimeSeconds() * 0.4f) * 1.2f);
}

// ---------------- camera (click only) ----------------

void AMarsGreenhousePlayerController::SelectCamera(int32 Index) { if (Index != CamIndex) SwitchCamera(Index); }

void AMarsGreenhousePlayerController::SwitchCamera(int32 Index)
{
	if (Cameras.Num() == 0) return;
	PendingCam = ((Index % Cameras.Num()) + Cameras.Num()) % Cameras.Num();
	PlaySfx(SfxTransition);
	if (PlayerCameraManager)
		PlayerCameraManager->StartCameraFade(0.f, 1.f, 0.12f, FColor::Black, false, true);
	GetWorldTimerManager().SetTimer(CamFadeTimer, this, &AMarsGreenhousePlayerController::FinishCameraSwitch, 0.12f, false);
}

void AMarsGreenhousePlayerController::FinishCameraSwitch()
{
	if (Cameras.IsValidIndex(PendingCam))
	{
		CamIndex = PendingCam;
		CamBaseRot = Cameras[CamIndex]->GetActorRotation();
		CamPanOffset = FRotator::ZeroRotator;
		SetViewTarget(Cameras[CamIndex]);
	}
	if (PlayerCameraManager)
		PlayerCameraManager->StartCameraFade(1.f, 0.f, 0.30f, FColor::Black, false, false);
}

void AMarsGreenhousePlayerController::DayFadeDip()
{
	PlaySfx(SfxTransition);
	if (PlayerCameraManager)
		PlayerCameraManager->StartCameraFade(0.f, 1.f, 0.12f, FColor::Black, false, true);
	GetWorldTimerManager().SetTimer(DayFadeTimer, this, &AMarsGreenhousePlayerController::FinishDayFade, 0.14f, false);
}

void AMarsGreenhousePlayerController::FinishDayFade()
{
	if (PlayerCameraManager)
		PlayerCameraManager->StartCameraFade(1.f, 0.f, 0.35f, FColor::Black, false, false);
}

// ---------------- sim reactions ----------------

void AMarsGreenhousePlayerController::HandleEventTriggered(FEventCard Card)
{
	PlaySfx(SfxAlert);
	const UMarsSimSettings* Cfg = GetDefault<UMarsSimSettings>();
	if (UClass* EvtClass = Cfg->EventPopupWidgetClass.LoadSynchronous())
	{
		if (UEventPopupWidget* Popup = CreateWidget<UEventPopupWidget>(this, EvtClass))
		{
			Popup->AddToViewport(10);
			Popup->Setup(Card);
		}
	}
}

void AMarsGreenhousePlayerController::HandleNewSol(int32 Sol) { DayFadeDip(); }
void AMarsGreenhousePlayerController::HandleHarvest(int32)    { PlaySfx(SfxConfirm); }
void AMarsGreenhousePlayerController::HandleFail(EResource)   { PlaySfx(SfxAlert); }
void AMarsGreenhousePlayerController::HandleWin()             { PlaySfx(SfxConfirm); }

// ---------------- world interaction ----------------

void AMarsGreenhousePlayerController::SelectBedFromWorld(int32 BedIndex)
{
	if (auto* S = GetSim())
		if (S->Beds.IsValidIndex(BedIndex)) { SelectedBed = BedIndex; PlaySfx(SfxClick); }
}

// ---------------- ONE dispatch for every clickable element ----------------

void AMarsGreenhousePlayerController::HandleHudAction(FName Action)
{
	const FString A = Action.ToString();
	if (A.StartsWith(TEXT("cam"))) { SelectCamera(FCString::Atoi(*A.Mid(3))); return; }
	if (A == TEXT("tut_next"))     { TutorialNext(); PlaySfx(SfxClick); return; }
	if (A == TEXT("tut_skip"))     { TutorialSkip(); PlaySfx(SfxClick); return; }

	UGreenhouseSimSubsystem* S = GetSim();
	if (!S) return;

	if      (A == TEXT("advance"))       { S->AdvanceSolNow(); PlaySfx(SfxClick); }
	else if (A == TEXT("restart"))       { S->RestartRun();    PlaySfx(SfxClick); }
	else if (A == TEXT("nextbed"))       { const int32 N = S->Beds.Num(); if (N > 0) SelectedBed = (SelectedBed + 1) % N; PlaySfx(SfxClick); }
	else if (A.StartsWith(TEXT("bed")))  { const int32 i = FCString::Atoi(*A.Mid(3)); if (S->Beds.IsValidIndex(i)) SelectedBed = i; PlaySfx(SfxClick); }
	else if (A == TEXT("led_purple"))    { S->SetLed(ELedColor::Purple);   PlaySfx(SfxClick); }
	else if (A == TEXT("led_white"))     { S->SetLed(ELedColor::White);    PlaySfx(SfxClick); }
	else if (A == TEXT("led_cycle"))     { S->CycleLed();                  PlaySfx(SfxClick); }
	else if (A == TEXT("act_water"))     { S->WaterPlant(SelectedBed);     PlaySfx(SfxConfirm); }
	else if (A == TEXT("act_ice"))       { S->MineIce();                   PlaySfx(SfxConfirm); }
	else if (A == TEXT("act_electro"))   { S->Electrolyze();               PlaySfx(SfxConfirm); }
	else if (A == TEXT("act_harvest"))   { S->HarvestBed(SelectedBed);     PlaySfx(SfxConfirm); }
	else if (A == TEXT("plant_potato"))  { S->PlantCrop(SelectedBed, ECropType::Potato);  PlaySfx(SfxConfirm); }
	else if (A == TEXT("plant_lettuce")) { S->PlantCrop(SelectedBed, ECropType::Lettuce); PlaySfx(SfxConfirm); }
	else if (A == TEXT("evt_a"))         { if (S->bEventActive) { S->ResolveEvent(0); PlaySfx(SfxClick); } }
	else if (A == TEXT("evt_b"))         { if (S->bEventActive && !S->CurrentEvent.bSingleChoice) { S->ResolveEvent(1); PlaySfx(SfxClick); } }

	TutorialTryAdvance(Action);
}

// ---------------- tutorial ----------------

FString AMarsGreenhousePlayerController::TutorialPrompt() const
{
	switch (TutorialStep)
	{
		case 0: return TEXT("Welcome to Bustan, Commander. Keep your crew alive for 15 sols on Mars.  (Click NEXT)");
		case 1: return TEXT("These 4 bars are life support - Oxygen, Water, Food, Power. If any hits zero, the colony dies. Power runs everything.  (Click NEXT)");
		case 2: return TEXT("Click a PLANTER (in the world or the list), then PLANT a crop: Potato (food) or Lettuce (fast, oxygen).");
		case 3: return TEXT("Set the GROW-LIGHT: Red + Blue = more oxygen, White = faster growth. Match it to your crop (potato likes white, lettuce likes red + blue).");
		case 4: return TEXT("Keep plants healthy - WATER them or they die. When a crop is READY, HARVEST it before it spoils. Short on a resource? Mine Ice or Electrolyze - both cost POWER.");
		case 5: return TEXT("Click END DAY to advance a sol. Story beats and crises will interrupt with a choice. Survive 15 sols. Good luck.");
		default: return FString();
	}
}

void AMarsGreenhousePlayerController::TutorialNext() { if (!bTutorialDone && TutorialStep <= 1) ++TutorialStep; }

void AMarsGreenhousePlayerController::TutorialTryAdvance(FName Action)
{
	if (bTutorialDone) return;
	const FString A = Action.ToString();
	switch (TutorialStep)
	{
		case 2: if (A.StartsWith(TEXT("plant_"))) ++TutorialStep; break;
		case 3: if (A.StartsWith(TEXT("led_")))   ++TutorialStep; break;
		case 4: if (A == TEXT("act_water") || A == TEXT("act_ice") || A == TEXT("act_electro") || A == TEXT("act_harvest")) ++TutorialStep; break;
		case 5: if (A == TEXT("advance")) bTutorialDone = true; break;
		default: break;
	}
}

// ---------------- input ----------------

void AMarsGreenhousePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AMarsGreenhousePlayerController::KeyAdvanceDay);
	InputComponent->BindKey(EKeys::R,     IE_Pressed, this, &AMarsGreenhousePlayerController::KeyRestart);
	InputComponent->BindKey(EKeys::Tab,   IE_Pressed, this, &AMarsGreenhousePlayerController::KeyNextBed);
	InputComponent->BindKey(EKeys::Z,     IE_Pressed, this, &AMarsGreenhousePlayerController::KeyPlantPotato);
	InputComponent->BindKey(EKeys::X,     IE_Pressed, this, &AMarsGreenhousePlayerController::KeyPlantLettuce);
	InputComponent->BindKey(EKeys::F,     IE_Pressed, this, &AMarsGreenhousePlayerController::KeyWater);
	InputComponent->BindKey(EKeys::E,     IE_Pressed, this, &AMarsGreenhousePlayerController::KeyHarvest);
	InputComponent->BindKey(EKeys::G,     IE_Pressed, this, &AMarsGreenhousePlayerController::KeyMineIce);
	InputComponent->BindKey(EKeys::H,     IE_Pressed, this, &AMarsGreenhousePlayerController::KeyElectrolyze);
	InputComponent->BindKey(EKeys::L,     IE_Pressed, this, &AMarsGreenhousePlayerController::KeyCycleLed);
	InputComponent->BindKey(EKeys::One,   IE_Pressed, this, &AMarsGreenhousePlayerController::KeyChoiceA);
	InputComponent->BindKey(EKeys::Two,   IE_Pressed, this, &AMarsGreenhousePlayerController::KeyChoiceB);
	// Camera keys intentionally removed - cameras switch by clicking their monitor thumbnails.
}

void AMarsGreenhousePlayerController::KeyAdvanceDay()   { HandleHudAction(FName("advance")); }
void AMarsGreenhousePlayerController::KeyRestart()      { HandleHudAction(FName("restart")); }
void AMarsGreenhousePlayerController::KeyNextBed()      { HandleHudAction(FName("nextbed")); }
void AMarsGreenhousePlayerController::KeyPlantPotato()  { HandleHudAction(FName("plant_potato")); }
void AMarsGreenhousePlayerController::KeyPlantLettuce() { HandleHudAction(FName("plant_lettuce")); }
void AMarsGreenhousePlayerController::KeyWater()        { HandleHudAction(FName("act_water")); }
void AMarsGreenhousePlayerController::KeyHarvest()      { HandleHudAction(FName("act_harvest")); }
void AMarsGreenhousePlayerController::KeyMineIce()      { HandleHudAction(FName("act_ice")); }
void AMarsGreenhousePlayerController::KeyElectrolyze()  { HandleHudAction(FName("act_electro")); }
void AMarsGreenhousePlayerController::KeyCycleLed()     { HandleHudAction(FName("led_cycle")); }
void AMarsGreenhousePlayerController::KeyChoiceA()      { HandleHudAction(FName("evt_a")); }
void AMarsGreenhousePlayerController::KeyChoiceB()      { HandleHudAction(FName("evt_b")); }
