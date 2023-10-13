// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "AA_AIRacerContext.generated.h"

class AAA_WheeledVehiclePawn;

/**
 * 
 */
USTRUCT(BlueprintType)
struct ALPINEASPHALT_API FAA_AIRacerContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	TObjectPtr<AAA_WheeledVehiclePawn> VehiclePawn{};

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	float DesiredSpeedMph{};

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	FVector MovementTarget{ EForceInit::ForceInitToZero };
};

/**
 *
 */
USTRUCT(BlueprintType)
struct ALPINEASPHALT_API FAA_AIRacerAvoidanceContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	float OffsetValue{};
};
