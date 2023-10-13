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
};
