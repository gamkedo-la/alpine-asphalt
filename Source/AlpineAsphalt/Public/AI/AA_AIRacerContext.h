// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Race/AA_RaceState.h"

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

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	float DesiredSpeedMph{};

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	FVector MovementTarget{ EForceInit::ForceInitToZero };

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	TObjectPtr<AAA_TrackInfoActor> RaceTrack{};

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	FAA_RaceState RaceState{};

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	float TargetDistanceAlongSpline {};

	void SetVehiclePawn(AAA_WheeledVehiclePawn* InVehiclePawn);
	void SetRaceTrack(AAA_TrackInfoActor* InRaceTrack);
};

struct ALPINEASPHALT_API FAA_AIRacerSnapshotData
{
	FAA_RaceState RaceState{};

	FVector MovementTarget{ EForceInit::ForceInitToZero };

	float DesiredSpeedMph{};
	float TargetDistanceAlongSpline{};
	bool bRaceCompleted{};
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
