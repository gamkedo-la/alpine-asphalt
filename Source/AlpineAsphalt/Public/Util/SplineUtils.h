// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class AAA_WheeledVehiclePawn;
class USplineComponent;
struct FAA_RaceState;

namespace AA::SplineUtils
{
	ALPINEASPHALT_API bool TryUpdateRaceState(const USplineComponent& Spline, FAA_RaceState& RaceState);
}
