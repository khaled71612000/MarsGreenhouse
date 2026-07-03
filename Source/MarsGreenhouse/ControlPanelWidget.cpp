// ControlPanelWidget.cpp
#include "ControlPanelWidget.h"
#include "GreenhouseSimSubsystem.h"
#include "MarsSimSettings.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UControlPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (UWorld* W = GetWorld()) Sim = W->GetSubsystem<UGreenhouseSimSubsystem>();
}

void UControlPanelWidget::NativeTick(const FGeometry& Geo, float DeltaTime)
{
	Super::NativeTick(Geo, DeltaTime);
	if (!Sim) return;

	if (OxygenBar) OxygenBar->SetPercent(Sim->Oxygen * 0.01f);
	if (WaterBar)  WaterBar->SetPercent(Sim->Water   * 0.01f);
	if (FoodBar)   FoodBar->SetPercent(Sim->Food     * 0.01f);
	if (PowerBar)  PowerBar->SetPercent(Sim->Power    * 0.01f);

	if (SolText)
	{
		const UMarsSimSettings* Cfg = GetDefault<UMarsSimSettings>();
		SolText->SetText(FText::FromString(FString::Printf(TEXT("SOL %d / %d"), Sim->Sol, Cfg->TotalSols)));
	}
	if (StateText)
		StateText->SetText(FText::FromString(StaticEnum<EGameState>()->GetNameStringByValue((int64)Sim->State)));
}
