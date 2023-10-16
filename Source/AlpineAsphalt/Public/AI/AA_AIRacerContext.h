// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "AA_AIRacerContext.generated.h"

class AAA_WheeledVehiclePawn;
class AAA_TrackInfoActor;

/**
 * 
 */
USTRUCT(BlueprintType)
struct ALPINEASPHALT_API FAA_AIRacerContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	TObjectPtr<AAA_WheeledVehiclePawn> VehiclePawn{};

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	float DesiredSpeedMph{};

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	FVector MovementTarget{ EForceInit::ForceInitToZero };

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	TObjectPtr<AAA_TrackInfoActor> RaceTrack{};

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	float DistanceAlongSpline {};
};

/**
 *
 */
USTRUCT(BlueprintType)
struct ALPINEASPHALT_API FAA_AIRacerAvoidanceContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	FVector ThreatVector{ EForceInit::ForceInitToZero };

	/*
	* Overall strength of threat [0,1].
	*/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	float NormalizedThreatScore{ };

	/*
	* Normalized speed of current threats.
	*/
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	float NormalizedThreatSpeedMph{ };

	int32 ThreatCount{};

	FString ToString() const;
};
