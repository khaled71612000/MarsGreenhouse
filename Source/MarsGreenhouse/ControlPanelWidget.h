// ControlPanelWidget.h — OPTIONAL C++ base for a stylized dashboard widget.
// Reparent your WBP_Dashboard to this and name any of the widgets below to auto-bind.
// All binds are OPTIONAL, so you can include only the ones you want.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ControlPanelWidget.generated.h"

class UGreenhouseSimSubsystem;
class UProgressBar;
class UTextBlock;

UCLASS()
class MARSGREENHOUSE_API UControlPanelWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& Geo, float DeltaTime) override;

	// Name these in your WBP to auto-fill them (all optional).
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> OxygenBar;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> WaterBar;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> FoodBar;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UProgressBar> PowerBar;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock>   SolText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock>   StateText;

private:
	UPROPERTY() TObjectPtr<UGreenhouseSimSubsystem> Sim;
};
