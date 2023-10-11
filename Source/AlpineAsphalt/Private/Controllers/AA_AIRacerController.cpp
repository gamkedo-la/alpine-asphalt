// Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/AA_AIRacerController.h"

#include "AI/AA_AIVehicleControlComponent.h"

AAA_AIRacerController::AAA_AIRacerController()
{
	VehicleControlComponent = CreateDefaultSubobject<UAA_AIVehicleControlComponent>(TEXT("Vehicle Control"));
}
