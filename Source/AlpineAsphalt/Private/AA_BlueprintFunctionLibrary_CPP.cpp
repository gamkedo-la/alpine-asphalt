// Fill out your copyright notice in the Description page of Project Settings.


#include "AA_BlueprintFunctionLibrary_CPP.h"

#include "AA_RoadSpline_CPP.h"
#include "LandscapeSplineActor.h"
#include "LandscapeSplinesComponent.h"
#include "LandscapeSplineSegment.h"

void UAA_BlueprintFunctionLibrary_CPP::GenerateRoadSpline(AActor* LandscapeSpline)
{
	//Cast Actor to Landscape Spline Actor
	ALandscapeSplineActor* LandscapeSplineActor = Cast<ALandscapeSplineActor>(LandscapeSpline);
	check(LandscapeSplineActor);
	
	//Cache World to spawn objects later
	UWorld* World = LandscapeSplineActor->GetWorld();

	//Get Segments and Control Points
	TArray<TObjectPtr<ULandscapeSplineSegment>> Segments = LandscapeSplineActor->GetSplinesComponent()->GetSegments();
	TArray<TObjectPtr<ULandscapeSplineControlPoint>> ControlPoints = LandscapeSplineActor->GetSplinesComponent()->GetControlPoints();

	UE_LOG(LogTemp,Warning,TEXT("%d"),Segments.Num());
	//Spawn Splines from landscape segments
	for (auto LandscapeSplineSegment : Segments)
	{

	}
	//World->SpawnActor<AAA_RoadSpline_CPP>();
	//Spawn Splines from Control Point connections
	
}
