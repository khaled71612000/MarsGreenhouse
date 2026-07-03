// EventPopupWidget.cpp
#include "EventPopupWidget.h"
#include "GreenhouseSimSubsystem.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UEventPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UWorld* W = GetWorld()) Sim = W->GetSubsystem<UGreenhouseSimSubsystem>();
	if (ChoiceAButton) ChoiceAButton->OnClicked.AddDynamic(this, &UEventPopupWidget::OnChoiceA);
	if (ChoiceBButton) ChoiceBButton->OnClicked.AddDynamic(this, &UEventPopupWidget::OnChoiceB);
}

void UEventPopupWidget::Setup(const FEventCard& Card)
{
	if (SpeakerText)   SpeakerText->SetText(Card.Speaker);
	if (SituationText) SituationText->SetText(Card.Situation);
	if (ChoiceALabel)  ChoiceALabel->SetText(Card.ChoiceA.Label);
	if (ChoiceBLabel)  ChoiceBLabel->SetText(Card.ChoiceB.Label);
}

void UEventPopupWidget::OnChoiceA() { if (Sim) Sim->ResolveEvent(0); RemoveFromParent(); }
void UEventPopupWidget::OnChoiceB() { if (Sim) Sim->ResolveEvent(1); RemoveFromParent(); }
