// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/AA_PlayerRaceSplineInfoComponent.h"

#include "VisualLogger/VisualLogger.h"
#include "Components/SplineComponent.h"

#include "Actors/AA_TrackInfoActor.h"
#include "Pawn/AA_WheeledVehiclePawn.h"
#include "Activity/AA_BaseActivity.h"

#include "Util/SplineUtils.h"
#include "Logging/LoggingUtils.h"
#include "Logging/AlpineAsphaltLogger.h"

using namespace AA;

UAA_PlayerRaceSplineInfoComponent::UAA_PlayerRaceSplineInfoComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickInterval = 0.2;
}

void UAA_PlayerRaceSplineInfoComponent::SetTrackInfo(AAA_TrackInfoActor* InTrackInfoActor)
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log, TEXT("%s-%s: SetTrackInfo: %s"), *LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(InTrackInfoActor));

	if(InTrackInfoActor == TrackInfoActor)
	{
		return;
	}

	if (InTrackInfoActor && !TrackInfoActor)
	{
		SetComponentTickEnabled(true);
	}
	else if(!InTrackInfoActor && TrackInfoActor)
	{
		SetComponentTickEnabled(false);
	}

	TrackInfoActor = InTrackInfoActor;
	PlayerSplineInfo.reset();
}

void UAA_PlayerRaceSplineInfoComponent::SetVehicle(const AAA_WheeledVehiclePawn* InVehicle)
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log, TEXT("%s-%s: SetVehicle: %s"), *LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(InVehicle));

	Vehicle = InVehicle;

	if (!InVehicle)
	{
		SetComponentTickEnabled(false);
	}
}

void UAA_PlayerRaceSplineInfoComponent::BeginPlay()
{
	Super::BeginPlay();

	RegisterRewindable(ERestoreTiming::Resume);
}

void UAA_PlayerRaceSplineInfoComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UnregisterRewindable();
}

void UAA_PlayerRaceSplineInfoComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Shouldn't happen unless actor is destroyed before Set...(nullptr) called

	if (!Vehicle)
	{
		TrackInfoActor = nullptr;
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Warning, TEXT("%s-%s: TickComponent - Detected Vehicle as nullptr!"), *LoggingUtils::GetName(GetOwner()), *GetName());

		SetComponentTickEnabled(false);
		return;
	}

	if (!TrackInfoActor)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Warning, TEXT("%s-%s: TickComponent - Detected TrackInfoActor as nullptr!"), *LoggingUtils::GetName(GetOwner()), *GetName());

		SetComponentTickEnabled(false);
		return;
	}

	UpdateSplineInfo();
}

void UAA_PlayerRaceSplineInfoComponent::UpdateSplineInfo()
{
	check(TrackInfoActor);
	check(Vehicle);

	auto Spline = TrackInfoActor->Spline;
	check(Spline);

	if (!PlayerSplineInfo)
	{
		// start of race - start behind the line
		PlayerSplineInfo = FPlayerSplineInfo
		{
			.RaceState = FAA_RaceState
			{
				.RaceTrack = TrackInfoActor,
				.VehiclePawn = Vehicle,
				.DistanceAlongSpline = 0,
				.SplineLength = Spline->GetSplineLength()
			}
		};
		return;
	}

	SplineUtils::TryUpdateRaceState(*Spline, PlayerSplineInfo->RaceState);
}

std::optional<FAA_RaceStateSnapshotData> UAA_PlayerRaceSplineInfoComponent::CaptureSnapshot() const
{
	if (!PlayerSplineInfo)
	{
		return std::nullopt;
	}

	const auto& RaceState = PlayerSplineInfo->RaceState;

	return FAA_RaceStateSnapshotData
	{
		.DistanceAlongSpline = RaceState.DistanceAlongSpline,
		.CurrentLapMaxCompletionFraction = RaceState.CurrentLapMaxCompletionFraction,
		.LapCount = RaceState.LapCount
	};
}

void UAA_PlayerRaceSplineInfoComponent::RestoreFromSnapshot(const std::optional<FAA_RaceStateSnapshotData>& InSnapshotData, float InRewindTime)
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
		TEXT("%s-%s: RestoreFromSnapshot: InRewindTime=%f"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), InRewindTime);

	if (!PlayerSplineInfo)
	{
		return;
	}

	auto& RaceState = PlayerSplineInfo->RaceState;

	if (InSnapshotData)
	{
		RaceState.DistanceAlongSpline = InSnapshotData->DistanceAlongSpline;
		RaceState.CurrentLapMaxCompletionFraction = InSnapshotData->CurrentLapMaxCompletionFraction;
		RaceState.LapCount = InSnapshotData->LapCount;
	}
	else
	{
		RaceState.DistanceAlongSpline = 0.0f;
		RaceState.CurrentLapMaxCompletionFraction = 0.0f;
		RaceState.LapCount = 0;
	}
}

#if ENABLE_VISUAL_LOG

void UAA_PlayerRaceSplineInfoComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("Player Race Spline Info Component");

	if(TrackInfoActor)
	{
		Category.Add(TEXT("Activity Type"), *LoggingUtils::GetName(TrackInfoActor->ActivityType));

		if (PlayerSplineInfo)
		{
			const auto& RaceState = PlayerSplineInfo->RaceState;

			Category.Add(TEXT("Lap"), FString::Printf(TEXT("%d"), RaceState.LapCount + 1));
			Category.Add(TEXT("Lap Completion %"), FString::Printf(TEXT("%.1f"), RaceState.GetCurrentLapCompletionFraction() * 100));
			Category.Add(TEXT("Lap DistanceAlongSpline"), FString::Printf(TEXT("%.1f"), RaceState.DistanceAlongSpline));
			Category.Add(TEXT("Total DistanceAlongSpline"), FString::Printf(TEXT("%.1f"), RaceState.GetTotalDistance()));
		}
	}

	Snapshot->Status.Add(Category);
}

#endif

