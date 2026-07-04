// GreenhouseLabel.cpp
#include "GreenhouseLabel.h"
#include "Components/BillboardComponent.h"

AGreenhouseLabel::AGreenhouseLabel()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Icon = CreateDefaultSubobject<UBillboardComponent>(TEXT("EditorIcon"));
	Icon->SetupAttachment(Root);
	Icon->SetHiddenInGame(true);
}
