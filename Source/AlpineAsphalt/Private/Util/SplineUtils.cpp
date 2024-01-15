#include "Util/SplineUtils.h"

#include "Components/SplineComponent.h"
#include "Pawn/AA_WheeledVehiclePawn.h"
#include "Race/AA_RaceState.h"
#include "Actors/AA_TrackInfoActor.h"
#include "Components/AA_CheckpointComponent.h"

#include "Logging/LoggingUtils.h"
#include "Logging/AlpineAsphaltLogger.h"
#include "VisualLogger/VisualLogger.h"

namespace
{
	constexpr float LapCompletionMinDelta = 0.1f; // Need to complete 90% of previous lap and now detect < 10% on next lap
	constexpr float NonCircuitMinCompletionFraction = 0.99f;

	const UObject* SplineUtilsGetLogOwner(const AAA_WheeledVehiclePawn& Vehicle);
	float GetMaxResetSplineDistance(const AAA_WheeledVehiclePawn& VehiclePawn, const FAA_RaceState& RaceState);
}

bool AA::SplineUtils::TryUpdateRaceState(const USplineComponent& Spline, FAA_RaceState& RaceState)
{
	check(RaceState.VehiclePawn);

	const auto& VehiclePawn = *RaceState.VehiclePawn;

	const auto Key = Spline.FindInputKeyClosestToWorldLocation(VehiclePawn.GetFrontWorldLocation());
	const auto CurrentDistanceAlongSpline = Spline.GetDistanceAlongSplineAtSplineInputKey(Key);
	const auto LastDistanceAlongSpline = RaceState.DistanceAlongSpline;
	const auto bIsLoop = RaceState.IsLooping();

	// TODO: Should refactor the lap completion logic to rely on checkpoints 
	// - would set the lap count and adjust completion percentage (for finish) on FRaceState outside this function

	// Keep at zero if still behind the starting line
	const auto DeltaFraction = (CurrentDistanceAlongSpline - LastDistanceAlongSpline) / RaceState.SplineLength;
	if (RaceState.LapCount == 0 && FMath::IsNearlyZero(RaceState.CurrentLapMaxCompletionFraction) && FMath::Abs(DeltaFraction) > 0.5f)
	{
		UE_VLOG_UELOG(SplineUtilsGetLogOwner(VehiclePawn), LogAlpineAsphalt, Verbose, TEXT("%s: TryUpdateRaceState - FALSE - CurrentLap=%d; CurrentDistanceAlongSpline=%f; LastDistanceAlongSpline=%f; DeltaFraction=%f > 0.5f"),
			*VehiclePawn.GetName(), RaceState.LapCount + 1, CurrentDistanceAlongSpline, LastDistanceAlongSpline, DeltaFraction);

		return false;
	}

	const auto CurrentCompletionFraction = CurrentDistanceAlongSpline / RaceState.SplineLength;
	const auto LastTotalDistance = RaceState.GetTotalDistance();

	if (CurrentCompletionFraction > RaceState.CurrentLapMaxCompletionFraction)
	{
		// complete if close to the end 
		if (!bIsLoop && CurrentCompletionFraction >= NonCircuitMinCompletionFraction)
		{
			UE_VLOG_UELOG(SplineUtilsGetLogOwner(VehiclePawn), LogAlpineAsphalt, Log,
				TEXT("%s: TryUpdateRaceState - TRUE - Completed non-closed loop lap %d: CurrentDistanceAlongSpline=%f; LastDistanceAlongSpline=%f; CurrentCompletionFraction=%f; CurrentLapMaxCompletionFraction=%f"),
				*VehiclePawn.GetName(), RaceState.LapCount + 1,
				CurrentDistanceAlongSpline, LastDistanceAlongSpline, CurrentCompletionFraction, RaceState.CurrentLapMaxCompletionFraction);

			RaceState.CurrentLapMaxCompletionFraction = 0;
			RaceState.LapCount = 1;
		}
		else
		{
			UE_VLOG_UELOG(SplineUtilsGetLogOwner(VehiclePawn), LogAlpineAsphalt, Verbose,
				TEXT("%s: TryUpdateRaceState - TRUE - Progress made on current lap %d (%f): CurrentDistanceAlongSpline=%f; LastDistanceAlongSpline=%f; CurrentCompletionFraction=%f > CurrentLapMaxCompletionFraction=%f;bIsLoop=%s"),
				*VehiclePawn.GetName(), RaceState.LapCount + 1, CurrentCompletionFraction - RaceState.CurrentLapMaxCompletionFraction,
				CurrentDistanceAlongSpline, LastDistanceAlongSpline, CurrentCompletionFraction, RaceState.CurrentLapMaxCompletionFraction,
				LoggingUtils::GetBoolString(bIsLoop));

			RaceState.CurrentLapMaxCompletionFraction = CurrentCompletionFraction;
		}
	}
	else if (bIsLoop && RaceState.CurrentLapMaxCompletionFraction >= 1 - LapCompletionMinDelta &&
		CurrentCompletionFraction < LapCompletionMinDelta)
	{
		// Completed the lap
		++RaceState.LapCount;

		UE_VLOG_UELOG(SplineUtilsGetLogOwner(VehiclePawn), LogAlpineAsphalt, Log,
			TEXT("%s: TryUpdateRaceState - TRUE - Completed lap %d: CurrentDistanceAlongSpline=%f; LastDistanceAlongSpline=%f; CurrentCompletionFraction=%f; CurrentLapMaxCompletionFraction=%f; bIsLoop=%s"),
			*VehiclePawn.GetName(), RaceState.LapCount,
			CurrentDistanceAlongSpline, LastDistanceAlongSpline, CurrentCompletionFraction, RaceState.CurrentLapMaxCompletionFraction,
			LoggingUtils::GetBoolString(bIsLoop));

		RaceState.CurrentLapMaxCompletionFraction = CurrentCompletionFraction;
	}
	else if(CurrentDistanceAlongSpline < RaceState.DistanceAlongSpline)
	{
		// backtracking - don't update the RaceState.CurrentLapMaxCompletionFraction
		UE_VLOG_UELOG(SplineUtilsGetLogOwner(VehiclePawn), LogAlpineAsphalt, Log,
			TEXT("%s: TryUpdateRaceState - TRUE - Going backwards on lap %d (%f): CurrentDistanceAlongSpline=%f; LastDistanceAlongSpline=%f; CurrentCompletionFraction=%f; CurrentLapMaxCompletionFraction=%f; bIsLoop=%s"),
			*VehiclePawn.GetName(), RaceState.LapCount + 1,
			CurrentCompletionFraction - RaceState.CurrentLapMaxCompletionFraction,
			CurrentDistanceAlongSpline, LastDistanceAlongSpline, CurrentCompletionFraction, RaceState.CurrentLapMaxCompletionFraction,
			LoggingUtils::GetBoolString(bIsLoop));
	}

	RaceState.DistanceAlongSpline = CurrentDistanceAlongSpline;
	if (RaceState.GetTotalDistance() > LastTotalDistance)
	{
		RaceState.MaxDistanceAlongSpline = CurrentDistanceAlongSpline;
	}

	return true;
}

AA::SplineUtils::FVehicleResetDetails AA::SplineUtils::GetLastSplineLocationDetails(const AAA_WheeledVehiclePawn& VehiclePawn, const USplineComponent& Spline, const FAA_RaceState& RaceState)
{
	// Don't reset too close to the race finish or the final checkpoint won't register. It is also a little unfair
	check(RaceState.RaceTrack);
	check(RaceState.RaceTrack->CheckpointComponent);

	const float ResetDistance = FMath::Min(RaceState.MaxDistanceAlongSpline, GetMaxResetSplineDistance(VehiclePawn, RaceState));

	return GetVehicleResetDetailsFromSplineDistance(Spline, ResetDistance);
}

AA::SplineUtils::FVehicleResetDetails AA::SplineUtils::GetVehicleResetDetailsFromSplineDistance(const USplineComponent& Spline, float DistanceAlongSpline)
{
	const auto& ResetWorldLocation = Spline.GetWorldLocationAtDistanceAlongSpline(DistanceAlongSpline);
	const auto& ResetWorldRotation = Spline.GetWorldDirectionAtDistanceAlongSpline(DistanceAlongSpline).Rotation();

	return
	{
		.WorldLocation = ResetWorldLocation,
		.WorldRotation = ResetWorldRotation,
		.SplineDistance = DistanceAlongSpline
	};
}

void AA::SplineUtils::ResetVehicleToLastSplineLocationDetails(AAA_WheeledVehiclePawn& VehiclePawn, const FVehicleResetDetails& VehicleResetDetails)
{
	UE_VLOG_UELOG(SplineUtilsGetLogOwner(VehiclePawn), LogAlpineAsphalt, Display,
		TEXT("%s: ResetVehicleToLastSplineLocation: %s with rotation %s"),
		*VehiclePawn.GetName(), *VehicleResetDetails.WorldLocation.ToCompactString(), *VehicleResetDetails.WorldRotation.ToCompactString());

	UE_VLOG_LOCATION(SplineUtilsGetLogOwner(VehiclePawn), LogAlpineAsphalt, Display,
		VehicleResetDetails.WorldLocation + FVector(0, 0, 75.0f), 150.0f, FColor::Blue,
		TEXT("%s: ResetVehicleLocation"), *VehiclePawn.GetName());

	VehiclePawn.ResetVehicleAtLocation(VehicleResetDetails.WorldLocation, VehicleResetDetails.WorldRotation);
}

FVector AA::SplineUtils::ResetVehicleToLastSplineLocation(AAA_WheeledVehiclePawn& VehiclePawn, const USplineComponent& Spline, const FAA_RaceState& RaceState)
{
	const auto& ResetDetails = GetLastSplineLocationDetails(VehiclePawn, Spline, RaceState);

	ResetVehicleToLastSplineLocationDetails(VehiclePawn, ResetDetails);

	return ResetDetails.WorldLocation;
}

float AA::SplineUtils::GetSplineLength(const AAA_TrackInfoActor& TrackInfo)
{
	// If it is a circuit race with more than 1 lap then just use the original spline length
	check(TrackInfo.Spline);
	check(TrackInfo.CheckpointComponent);

	if (TrackInfo.LapsToComplete != 1)
	{
		return TrackInfo.Spline->GetSplineLength();
	}

	return TrackInfo.CheckpointComponent->GetDistanceAlongSplineAtRaceFinish();
}

namespace
{
	const UObject* SplineUtilsGetLogOwner(const AAA_WheeledVehiclePawn& Vehicle)
	{
		if (auto Controller = Vehicle.GetController(); Controller)
		{
			return Controller;
		}

		return &Vehicle;
	}

	float GetMaxResetSplineDistance(const AAA_WheeledVehiclePawn& VehiclePawn, const FAA_RaceState& RaceState)
	{
		// Don't reset too close to the race finish or the final checkpoint won't register. It is also a little unfair
		check(RaceState.RaceTrack);
		check(RaceState.RaceTrack->CheckpointComponent);

		const float VehicleBuffer = VehiclePawn.GetVehicleLength() * 5;
		float DistanceEnd = RaceState.SplineLength - VehicleBuffer;

		const auto& CheckpointData = RaceState.RaceTrack->CheckpointComponent->CheckpointPositionData;
		if (!CheckpointData.IsEmpty())
		{
			DistanceEnd -= CheckpointData.Last().Depth;
		}

		return FMath::Max(0, DistanceEnd);
	}
}
