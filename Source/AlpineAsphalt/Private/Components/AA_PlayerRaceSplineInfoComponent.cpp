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
				.VehiclePawn = Vehicle,
				.DistanceAlongSpline = 0,
				.SplineLength = Spline->GetSplineLength()
			}
		};
		return;
	}

	SplineUtils::TryUpdateSplineDistance(*Spline, *Vehicle, PlayerSplineInfo->RaceState.DistanceAlongSpline, PlayerSplineInfo->RaceState.DistanceAlongSpline);
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
			Category.Add(TEXT("DistanceAlongSpline"), PlayerSplineInfo ? FString::Printf(TEXT("%.1f"), PlayerSplineInfo->RaceState.DistanceAlongSpline) : TEXT("?"));
		}
	}

	Snapshot->Status.Add(Category);
}

#endif

