// Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/AA_AIRacerController.h"

#include "AI/AA_AIVehicleControlComponent.h"
#include "AI/AA_RacerSplineFollowingComponent.h"
#include "AI/AA_ObstacleDetectionComponent.h"
#include "AI/AA_RacerObstacleAvoidanceComponent.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/AlpineAsphaltLogger.h"
#include "Logging/LoggingUtils.h"
#include "Landscape.h"
#include "Components/SplineComponent.h"

#include "Actors/AA_TrackInfoActor.h"
#include "Pawn/AA_WheeledVehiclePawn.h"

#include <limits>

using namespace AA;

namespace
{
	constexpr float AIRacerControllerOneShotTimerDelay = 0.25f;
}

AAA_AIRacerController::AAA_AIRacerController()
{
	VehicleControlComponent = CreateDefaultSubobject<UAA_AIVehicleControlComponent>(TEXT("Vehicle Control"));
	RacerSplineFollowingComponent = CreateDefaultSubobject<UAA_RacerSplineFollowingComponent>(TEXT("Racer Spline Following"));
	ObstacleDetectionComponent = CreateDefaultSubobject<UAA_ObstacleDetectionComponent>(TEXT("Obstacle Detection"));
	RacerObstacleAvoidanceComponent = CreateDefaultSubobject<UAA_RacerObstacleAvoidanceComponent>(TEXT("Racer Obstacle Avoidance"));
}

void AAA_AIRacerController::SetTrackInfo(AAA_TrackInfoActor* TrackInfoActor)
{
	RacerContext.RaceTrack = TrackInfoActor;
}

#if ENABLE_VISUAL_LOG
void AAA_AIRacerController::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	Super::GrabDebugSnapshot(Snapshot);

	auto& Category = Snapshot->Status[0];
	Category.Add(TEXT("Race Track"), *LoggingUtils::GetName(RacerContext.RaceTrack));

	if (VehicleControlComponent)
	{
		VehicleControlComponent->DescribeSelfToVisLog(Snapshot);
	}

	if (RacerSplineFollowingComponent)
	{
		RacerSplineFollowingComponent->DescribeSelfToVisLog(Snapshot);
	}

	if (ObstacleDetectionComponent)
	{
		ObstacleDetectionComponent->DescribeSelfToVisLog(Snapshot);
	}

	if (RacerObstacleAvoidanceComponent)
	{
		RacerObstacleAvoidanceComponent->DescribeSelfToVisLog(Snapshot);
	}
}
#endif

void AAA_AIRacerController::BeginPlay()
{
	UE_VLOG_UELOG(this, LogAlpineAsphalt, Log, TEXT("%s: BeginPlay"), *GetName());

	Super::BeginPlay();
}

void AAA_AIRacerController::OnPossess(APawn* InPawn)
{
	UE_VLOG_UELOG(this, LogAlpineAsphalt, Log, TEXT("%s: OnPossess: %s"), *GetName(), *LoggingUtils::GetName(InPawn));

	Super::OnPossess(InPawn);

	RacerContext = {};

	auto VehiclePawn = Cast<AAA_WheeledVehiclePawn>(InPawn);

	if (!VehiclePawn)
	{
		UE_VLOG_UELOG(this, LogAlpineAsphalt, Error, TEXT("%s: OnPossess: %s (%s) is not a AAA_WheeledVehiclePawn!"),
			*GetName(), *LoggingUtils::GetName(InPawn), InPawn ? *LoggingUtils::GetName(InPawn->GetClass()) : TEXT("NULL"));
		return;
	}

	// The vehicle changes parameters on start so defer setting these 
	FTimerHandle OneShotTimer;
	
	GetWorldTimerManager().SetTimer(OneShotTimer, this, &ThisClass::SetVehicleParameters, AIRacerControllerOneShotTimerDelay);

	if(!RacerContext.RaceTrack)
	{
		SetRaceTrack(*VehiclePawn);
	}

	check(VehicleControlComponent);
	VehicleControlComponent->SetVehiclePawn(VehiclePawn);

	RacerContext.VehiclePawn = VehiclePawn;
	RacerContext.DesiredSpeedMph = VehicleControlComponent->GetDesiredSpeedMph();

	// TODO: Plan to use AI State Tree to manage AI behavior.  Since this is new to UE 5.1 and haven't used it, 
	// fallback would be to use a behavior tree or even just code up the logic in the AI Controller itself
	
	SetupComponentEventBindings();

	UE_VLOG_UELOG(this, LogAlpineAsphalt, Log, TEXT("%s: OnPossess: RaceTrack=%s"),
		*GetName(), *LoggingUtils::GetName(RacerContext.RaceTrack));
}

void AAA_AIRacerController::OnUnPossess()
{
	UE_VLOG_UELOG(this, LogAlpineAsphalt, Log, TEXT("%s: OnUnPossess: PreviousPawn=%s"), *GetName(), *LoggingUtils::GetName(GetPawn()));

	Super::OnUnPossess();

	RacerContext = {};

	if (VehicleControlComponent)
	{
		VehicleControlComponent->SetVehiclePawn(nullptr);
	}
}

void AAA_AIRacerController::SetupComponentEventBindings()
{
	check(VehicleControlComponent);
	check(RacerObstacleAvoidanceComponent);
	check(RacerSplineFollowingComponent);

	VehicleControlComponent->OnVehicleReachedTarget.AddDynamic(RacerSplineFollowingComponent, &UAA_RacerSplineFollowingComponent::SelectNewMovementTarget);
	RacerSplineFollowingComponent->OnVehicleTargetUpdated.AddDynamic(VehicleControlComponent, &UAA_AIVehicleControlComponent::OnVehicleTargetUpdated);
	RacerObstacleAvoidanceComponent->OnVehicleAvoidancePositionUpdated.AddDynamic(RacerSplineFollowingComponent, &UAA_RacerSplineFollowingComponent::OnVehicleAvoidancePositionUpdated);
}

void AAA_AIRacerController::SetRaceTrack(const AAA_WheeledVehiclePawn& VehiclePawn)
{
	// TODO: This will probably be passed in externally
	// here we just find the nearest one that has smallest completion percentage

	const auto GameWorld = GetWorld();

	AAA_TrackInfoActor* NearestRaceTrack{};
	double SmallestCompletion{ std::numeric_limits<double>::max() };

	const auto& VehicleLocation = VehiclePawn.GetActorLocation();

	for (TObjectIterator<AAA_TrackInfoActor> It; It; ++It)
	{
		if (GameWorld != It->GetWorld() || !It->Spline)
		{
			continue;
		}

		AAA_TrackInfoActor* RaceTrack = *It;
		auto Spline = RaceTrack->Spline;

		const auto NearestSplineLocation = Spline->FindLocationClosestToWorldLocation(VehicleLocation, ESplineCoordinateSpace::World);
		const double DistMeters = FVector::Distance(VehicleLocation, NearestSplineLocation) / 100;
		// Don't find one too far away
		if (DistMeters > MaxRaceDistance)
		{
			continue;
		}

		const auto Key = Spline->FindInputKeyClosestToWorldLocation(NearestSplineLocation);
		const auto DistanceAlongSpline = Spline->GetDistanceAlongSplineAtSplineInputKey(Key);
		const auto CompletionFraction = DistanceAlongSpline / Spline->GetSplineLength();
			
		if (CompletionFraction < SmallestCompletion)
		{
			NearestRaceTrack = RaceTrack;
			SmallestCompletion = CompletionFraction;
		}
	}

	RacerContext.RaceTrack = NearestRaceTrack;
	
	if (NearestRaceTrack)
	{
		UE_VLOG_UELOG(this, LogAlpineAsphalt, Log, TEXT("%s: SetRaceTrack: NearestRaceTrack=%s"),
			*GetName(), *NearestRaceTrack->GetName());
	}
	else
	{
		UE_VLOG_UELOG(this, LogAlpineAsphalt, Error, TEXT("%s: SetRaceTrack: NearestRaceTrack could not be determined!"),
			*GetName());
	}
}

void AAA_AIRacerController::SetVehicleParameters()
{
	auto VehiclePawn = RacerContext.VehiclePawn;

	if (!VehiclePawn)
	{
		return;
	}

	VehiclePawn->SetTractionControlState(true);
	VehiclePawn->SetABSState(true);
}
