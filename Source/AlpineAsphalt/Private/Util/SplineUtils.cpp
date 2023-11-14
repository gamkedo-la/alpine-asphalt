#include "Util/SplineUtils.h"

#include "Components/SplineComponent.h"
#include "Pawn/AA_WheeledVehiclePawn.h"
#include "Race/AA_RaceState.h"
#include "Logging/LoggingUtils.h"
#include "Logging/AlpineAsphaltLogger.h"
#include "VisualLogger/VisualLogger.h"

namespace
{
	constexpr float LapCompletionMinDelta = 0.1f; // Need to complete 90% of previous lap and now detect < 10% on next lap
	constexpr float NonCircuitMinCompletionFraction = 0.99f;

	const UObject* SplineUtilsGetLogOwner(const AAA_WheeledVehiclePawn& Vehicle);
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

	return true;
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
}
