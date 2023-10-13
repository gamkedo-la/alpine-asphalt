// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AA_RacerObstacleAvoidanceComponent.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/AlpineAsphaltLogger.h"
#include "Logging/LoggingUtils.h"
#include "AI/AA_RacerContextProvider.h"

using namespace AA;

// Sets default values for this component's properties
UAA_RacerObstacleAvoidanceComponent::UAA_RacerObstacleAvoidanceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAA_RacerObstacleAvoidanceComponent::OnVehicleObstaclesUpdated(AAA_WheeledVehiclePawn* VehiclePawn, const TArray<AAA_WheeledVehiclePawn*>& VehicleObstacles)
{
}


// Called when the game starts
void UAA_RacerObstacleAvoidanceComponent::BeginPlay()
{
	Super::BeginPlay();

	RacerContextProvider = Cast<IAA_RacerContextProvider>(GetOwner());

	if (!RacerContextProvider)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Error,
			TEXT("%s-s: BeginPlay - Owner does not implement IAA_RacerContextProvider"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
		return;
	}

}

#if ENABLE_VISUAL_LOG

void UAA_RacerObstacleAvoidanceComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("Racer Obstacle Avoidance Component");

	Snapshot->Status.Add(Category);
}

#endif
