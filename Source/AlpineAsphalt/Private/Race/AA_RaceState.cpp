#include "Race/AA_RaceState.h"

#include "Pawn/AA_WheeledVehiclePawn.h"
#include "Actors/AA_TrackInfoActor.h"
#include "Logging/LoggingUtils.h"

using namespace AA;

FString FAA_RaceState::ToString() const
{
	return FString::Printf(TEXT("RaceTrack=%s; VehiclePawn=%s; DistanceAlongSpline=%f; SplineLength=%f; LapCount=%d; CurrentLapMaxCompletionFraction=%f"),
		*LoggingUtils::GetName(RaceTrack), *LoggingUtils::GetName(VehiclePawn),
		DistanceAlongSpline, SplineLength, LapCount, CurrentLapMaxCompletionFraction
	);
}

int32 FAA_RaceState::GetNumLaps() const
{
	check(RaceTrack);
	return IsLooping() ? RaceTrack->LapsToComplete : 1;
}

bool FAA_RaceState::IsLooping() const
{
	check(RaceTrack);
	return RaceTrack->IsCircuit;
}