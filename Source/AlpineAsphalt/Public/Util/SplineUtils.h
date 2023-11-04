// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class AAA_WheeledVehiclePawn;
class USplineComponent;

namespace AA::SplineUtils
{
	ALPINEASPHALT_API bool TryUpdateSplineDistance(const USplineComponent& Spline, const AAA_WheeledVehiclePawn& Vehicle, float LastDistanceAlongSpline, float& NewDistanceAlongSpline);
}
