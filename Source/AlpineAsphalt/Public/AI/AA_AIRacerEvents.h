// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "AA_AIRacerEvents.generated.h"

class AAA_WheeledVehiclePawn;
struct FAA_AIRacerAvoidanceContext;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVehicleReachedTarget, AAA_WheeledVehiclePawn*, VehiclePawn, const FVector&, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnVehicleTargetUpdated, AAA_WheeledVehiclePawn*, VehiclePawn, const FVector&, Target, float, NewDesiredSpeedMph);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVehicleObstaclesUpdated, AAA_WheeledVehiclePawn*, VehiclePawn, const TArray<AAA_WheeledVehiclePawn*>&, VehicleObstacles);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVehicleAvoidancePositionUpdated, AAA_WheeledVehiclePawn*, VehiclePawn, const FAA_AIRacerAvoidanceContext&, AvoidanceContext);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVehicleStuck, AAA_WheeledVehiclePawn*, VehiclePawn, const FVector&, IdealTarget);

// This is required in order for the above delegate declaration to code expand - otherwise the macros do nothing
UCLASS()
class UAA_AIRacerEventsDummyClass : public UObject
{
	GENERATED_BODY()
};
