// EventPopupWidget.h — OPTIONAL C++ base for a stylized event popup.
// Reparent your WBP_EventPopup to this; name the widgets below (all optional).
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MarsSimTypes.h"
#include "EventPopupWidget.generated.h"

class UGreenhouseSimSubsystem;
class UTextBlock;
class UButton;

UCLASS()
class MARSGREENHOUSE_API UEventPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Called by the player controller right after spawning; fills the text.
	UFUNCTION(BlueprintCallable, Category="Event")
	void Setup(const FEventCard& Card);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> SpeakerText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> SituationText;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> ChoiceALabel;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> ChoiceBLabel;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton>    ChoiceAButton;
	UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton>    ChoiceBButton;

private:
	UPROPERTY() TObjectPtr<UGreenhouseSimSubsystem> Sim;
	UFUNCTION() void OnChoiceA();
	UFUNCTION() void OnChoiceB();
};
