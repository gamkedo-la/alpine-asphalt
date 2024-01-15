// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class AAA_WheeledVehiclePawn;
class USplineComponent;
struct FAA_RaceState;
class AAA_TrackInfoActor;

namespace AA::SplineUtils
{
	struct ALPINEASPHALT_API FVehicleResetDetails
	{
		FVector WorldLocation{};
		FRotator WorldRotation{};
		float SplineDistance{};
	};

	ALPINEASPHALT_API bool TryUpdateRaceState(const USplineComponent& Spline, FAA_RaceState& RaceState);

	ALPINEASPHALT_API FVehicleResetDetails GetLastSplineLocationDetails(const AAA_WheeledVehiclePawn& VehiclePawn, const USplineComponent& Spline, const FAA_RaceState& RaceState);

	ALPINEASPHALT_API FVehicleResetDetails GetVehicleResetDetailsFromSplineDistance(const USplineComponent& Spline, float DistanceAlongSpline);

	ALPINEASPHALT_API void ResetVehicleToLastSplineLocationDetails(AAA_WheeledVehiclePawn& VehiclePawn, const FVehicleResetDetails& VehicleResetDetails);

	ALPINEASPHALT_API FVector ResetVehicleToLastSplineLocation(AAA_WheeledVehiclePawn& VehiclePawn, const USplineComponent& Spline, const FAA_RaceState& RaceState);

	ALPINEASPHALT_API float GetSplineLength(const AAA_TrackInfoActor& TrackInfo);
}
