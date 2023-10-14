// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AA_ObstacleDetectionComponent.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/AlpineAsphaltLogger.h"
#include "Logging/LoggingUtils.h"
#include "AI/AA_RacerContextProvider.h"

using namespace AA;

UAA_ObstacleDetectionComponent::UAA_ObstacleDetectionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.5f;
}


void UAA_ObstacleDetectionComponent::BeginPlay()
{
	Super::BeginPlay();

	// TODO: Extract this into a base component class implemented by all the Racer AI components
	RacerContextProvider = Cast<IAA_RacerContextProvider>(GetOwner());

	if (!RacerContextProvider)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Error,
			TEXT("%s-%s: BeginPlay - Owner does not implement IAA_RacerContextProvider"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
		return;
	}
	
}


void UAA_ObstacleDetectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

#if ENABLE_VISUAL_LOG

void UAA_ObstacleDetectionComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("AI Obstacle Detection");

	Snapshot->Status.Add(Category);
}

#endif

