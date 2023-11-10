#include "Race/AA_RaceState.h"

#include "Pawn/AA_WheeledVehiclePawn.h"
#include "Logging/LoggingUtils.h"

FString FAA_RaceState::ToString() const
{
	return FString::Printf(TEXT("VehiclePawn=%s; DistanceAlongSpline=%f; SplineLength=%f; LapCount=%d"),
		*AA::LoggingUtils::GetName(VehiclePawn), DistanceAlongSpline, SplineLength, LapCount);
}
