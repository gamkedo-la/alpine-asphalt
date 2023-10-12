// Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/AA_AIRacerController.h"

#include "AI/AA_AIVehicleControlComponent.h"

AAA_AIRacerController::AAA_AIRacerController()
{
	VehicleControlComponent = CreateDefaultSubobject<UAA_AIVehicleControlComponent>(TEXT("Vehicle Control"));
}

#if ENABLE_VISUAL_LOG
void AAA_AIRacerController::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	Super::GrabDebugSnapshot(Snapshot);

	if (VehicleControlComponent)
	{
		VehicleControlComponent->DescribeSelfToVisLog(Snapshot);
	}
}
#endif
