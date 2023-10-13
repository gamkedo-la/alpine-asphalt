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

#include "Pawn/AA_WheeledVehiclePawn.h"

using namespace AA;

AAA_AIRacerController::AAA_AIRacerController()
{
	VehicleControlComponent = CreateDefaultSubobject<UAA_AIVehicleControlComponent>(TEXT("Vehicle Control"));
	RacerSplineFollowingComponent = CreateDefaultSubobject<UAA_RacerSplineFollowingComponent>(TEXT("Racer Spline Following"));
	ObstacleDetectionComponent = CreateDefaultSubobject<UAA_ObstacleDetectionComponent>(TEXT("Obstacle Detection"));
	RacerObstacleAvoidanceComponent = CreateDefaultSubobject<UAA_RacerObstacleAvoidanceComponent>(TEXT("Racer Obstacle Avoidance"));
}

#if ENABLE_VISUAL_LOG
void AAA_AIRacerController::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	Super::GrabDebugSnapshot(Snapshot);

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

	check(VehicleControlComponent);
	VehicleControlComponent->SetVehiclePawn(VehiclePawn);

	RacerContext.VehiclePawn = VehiclePawn;
	RacerContext.DesiredSpeedMph = VehicleControlComponent->GetDesiredSpeedMph();

	// TODO: Plan to use AI State Tree to manage AI behavior.  Since this is new to UE 5.1 and haven't used it, 
	// fallback would be to use a behavior tree or even just code up the logic in the AI Controller itself
	
	SetupComponentEventBindings();
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

#endif
