// Fill out your copyright notice in the Description page of Project Settings.


#include "AA_BlueprintFunctionLibrary_CPP.h"
#include "AA_RoadSpline_CPP.h"
#include "LandscapeSplineActor.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/Internal/Kismet/BlueprintTypeConversions.h"

void UAA_BlueprintFunctionLibrary_CPP::GenerateRoadSpline(AActor* LandscapeSpline)
{
	//Cast Actor to Landscape Spline Actor
	const ALandscapeSplineActor* LandscapeSplineActor = Cast<ALandscapeSplineActor>(LandscapeSpline);
	check(LandscapeSplineActor);
	
	//Cache World to spawn objects later
	UWorld* World = LandscapeSplineActor->GetWorld();
	
	//Add SplineMeshComponents to Maps
	TMap<FIntVector, USplineMeshComponent*> StartMap;
	TMap<FIntVector, USplineMeshComponent*> FinishMap; 
	for (auto Component : LandscapeSplineActor->GetComponents())
	{
		if(Component->IsA<USplineMeshComponent>())
		{
			USplineMeshComponent* SplineComponent = Cast<USplineMeshComponent>(Component);
			FVector Startf = SplineComponent->SplineParams.StartPos + SplineComponent->GetComponentLocation();
			FVector Endf = SplineComponent->SplineParams.StartPos + SplineComponent->GetComponentLocation();
			FIntVector Start = FIntVector(FMath::CeilToInt(Startf.X),FMath::CeilToInt(Startf.Y),FMath::CeilToInt(Startf.Z));
			FIntVector End = FIntVector(FMath::CeilToInt(Endf.X),FMath::CeilToInt(Endf.Y),FMath::CeilToInt(Endf.Z));
			StartMap.Add(Start,SplineComponent);
			FinishMap.Add(End,SplineComponent);

			UE_LOG(LogTemp,Warning,TEXT("Adding StartMap at %s"), *Start.ToString())
			UE_LOG(LogTemp,Warning,TEXT("Adding EndMap at %s"),  *End.ToString())
		}
	}

	
	AAA_RoadSpline_CPP* RoadSpline = World->SpawnActor<AAA_RoadSpline_CPP>();
	RoadSpline->RoadSpline->ClearSplinePoints();
	RoadSpline->SetFolderPath("RoadSplines");

	auto It= StartMap.CreateIterator();
	
	RoadSpline->RoadSpline->AddSplinePoint(It.Value()->SplineParams.StartPos + It.Value()->GetComponentLocation(),ESplineCoordinateSpace::World,true);
	RoadSpline->RoadSpline->SetTangentAtSplinePoint(0,It.Value()->SplineParams.StartTangent,ESplineCoordinateSpace::World);
	USplineMeshComponent** NextSpline = &It.Value();
	while(NextSpline)
	{
		UE_LOG(LogTemp,Warning,TEXT("Adding Point at %s"), *((*NextSpline)->SplineParams.EndPos + (*NextSpline)->GetComponentLocation()).ToString())
		RoadSpline->RoadSpline->AddSplinePoint((*NextSpline)->SplineParams.EndPos + (*NextSpline)->GetComponentLocation(),ESplineCoordinateSpace::World,true);
		RoadSpline->RoadSpline->SetTangentAtSplinePoint(RoadSpline->RoadSpline->GetNumberOfSplinePoints()-1,(*NextSpline)->SplineParams.EndTangent,ESplineCoordinateSpace::World);
		FVector Nextf = (*NextSpline)->SplineParams.EndPos + (*NextSpline)->GetComponentLocation();
		FIntVector Next = FIntVector(FMath::CeilToInt(Nextf.X),FMath::CeilToInt(Nextf.Y),FMath::CeilToInt(Nextf.Z));
		UE_LOG(LogTemp,Warning,TEXT("Looking for Next at %s"),  *Next.ToString())
		NextSpline = (StartMap.Find(Next));
		if(NextSpline && NextSpline == &It.Value()){ break; }
	}
	
	UE_LOG(LogTemp,Warning,TEXT("Done"))
}
