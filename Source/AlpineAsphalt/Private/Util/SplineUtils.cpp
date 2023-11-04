#include "Util/SplineUtils.h"

#include "Components/SplineComponent.h"
#include "Pawn/AA_WheeledVehiclePawn.h"

bool AA::SplineUtils::TryUpdateSplineDistance(const USplineComponent& Spline, const AAA_WheeledVehiclePawn& Vehicle, float LastDistanceAlongSpline, float& NewDistanceAlongSpline)
{
	const auto Key = Spline.FindInputKeyClosestToWorldLocation(Vehicle.GetFrontWorldLocation());
	const auto DistanceAlongSpline = Spline.GetDistanceAlongSplineAtSplineInputKey(Key);

	// Keep at zero if still behind the starting line
	const auto Delta = (DistanceAlongSpline - LastDistanceAlongSpline) / Spline.GetSplineLength();
	if (FMath::Abs(Delta) > 0.5f)
	{
		return false;
	}

	NewDistanceAlongSpline = DistanceAlongSpline;

	return true;
}
