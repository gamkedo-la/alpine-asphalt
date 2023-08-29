// Fill out your copyright notice in the Description page of Project Settings.


#include "AA_BlueprintFunctionLibrary_CPP.h"

#include "AA_RaceSpline_CPP.h"
#include "AA_RoadSpline_CPP.h"
#include "LandscapeSplineActor.h"
#include "LandscapeSplineControlPoint.h"
#include "Components/SplineComponent.h"
#include "Engine/StaticMeshSocket.h"

/**
 * @brief
 * Generates AA_RoadSplines_CPP for the given Landscape Spline
 * @param LandscapeSpline
 * @param RoadSplineBP
 */
void UAA_BlueprintFunctionLibrary_CPP::GenerateRoadSpline(AActor* LandscapeSpline, TSubclassOf<AAA_RoadSpline_CPP> RoadSplineBP)
{
	//Cast Actor to Landscape Spline Actor
	const ALandscapeSplineActor* LandscapeSplineActor = Cast<ALandscapeSplineActor>(LandscapeSpline);
	check(LandscapeSplineActor);
	
	//Cache World to spawn objects later
	UWorld* World = LandscapeSplineActor->GetWorld();

	TArray<TObjectPtr<ULandscapeSplineSegment>> Segments = LandscapeSplineActor->GetSplinesComponent()->GetSegments();

	//for every segment
	for (const TObjectPtr<ULandscapeSplineSegment> Segment : Segments)
	{
		//Create a new spline actor
		AAA_RoadSpline_CPP* RoadSpline = World->SpawnActor<AAA_RoadSpline_CPP>(
			RoadSplineBP,
			Segment->Connections[0].ControlPoint->Location + LandscapeSplineActor->GetActorLocation(),
			Segment->Connections[0].ControlPoint->Rotation + LandscapeSplineActor->GetActorRotation());

		//Clear premade points
		RoadSpline->Spline->ClearSplinePoints();
	
#if WITH_EDITOR
		RoadSpline->SetFolderPath("RoadSplines");
#endif
		
		for(int i = 0; i < 2; i++) //There are always two connections, Start and End, add a point for each
			{
			FLandscapeSplineSegmentConnection Connection = Segment->Connections[i];
			//Get control point location, rotation, and forward vector (Tangent Line)
			FVector Location = Connection.ControlPoint->Location + LandscapeSplineActor->GetActorLocation();
			FRotator Rotation = Connection.ControlPoint->Rotation + LandscapeSplineActor->GetActorRotation();
			FVector ForwardVector = Connection.ControlPoint->Rotation.Vector();
			
			//If attached to a socket, add it's location and rotation
			if(Connection.SocketName != "" && Connection.ControlPoint->Mesh)
			{
				if(const UStaticMeshSocket* Socket = Connection.ControlPoint->Mesh->FindSocket(Connection.SocketName))
				{
					Location += Rotation.RotateVector(Socket->RelativeLocation*Connection.ControlPoint->MeshScale);
					Rotation += Socket->RelativeRotation;
					ForwardVector = Socket->RelativeRotation.RotateVector(ForwardVector);
					ForwardVector.Normalize(); // needs to be normalized after rotation
				}
			}
			//The ending Vector needs to be flipped directions or else it goes 180 degrees the wrong way
			if(i == 1)
			{
				ForwardVector *= -1;
			}

			//Add the point with tangent line (From the Forward Vector of the Control Point)
			RoadSpline->Spline->AddSplinePoint(Location, ESplineCoordinateSpace::World,true);
			RoadSpline->Spline->SetTangentAtSplinePoint(RoadSpline->Spline->GetNumberOfSplinePoints()-1,
				ForwardVector*Segment->Connections[i].TangentLen,
				ESplineCoordinateSpace::World);
			}
	}

	 
	//Add Intersection connections
	for (const auto ControlPoint : LandscapeSplineActor->GetSplinesComponent()->GetControlPoints())
	{
		if(!ControlPoint->Mesh){continue;} //only an intersection if it has a mesh
		float TangentSize = ControlPoint->Mesh->GetBoundingBox().GetExtent().GetMax()*2;
		TangentSize *= ControlPoint->MeshScale.GetMax();
		TArray<TObjectPtr<UStaticMeshSocket>> Sockets = ControlPoint->Mesh->Sockets;
		//connect sockets together
		for(int i = 0; i < Sockets.Num(); i++)
		{
			for(int j = i + 1; j < Sockets.Num(); j++)
			{
				//Create a new spline actor
				AAA_RoadSpline_CPP* RoadSpline = World->SpawnActor<AAA_RoadSpline_CPP>(
					RoadSplineBP,
					ControlPoint->Location + LandscapeSplineActor->GetActorLocation(),
					ControlPoint->Rotation + LandscapeSplineActor->GetActorRotation());
				RoadSpline->Spline->ClearSplinePoints();

#if WITH_EDITOR
				RoadSpline->SetFolderPath("RoadSplines");
#endif
				//Spawn point at i///////////////////////////////////////////////////////
				
				//Get control point location, rotation, and forward vector (Tangent Line)
				FVector Location = ControlPoint->Location + LandscapeSplineActor->GetActorLocation();
				FRotator Rotation = ControlPoint->Rotation + LandscapeSplineActor->GetActorRotation();
				FVector ForwardVector = ControlPoint->Rotation.Vector();
				Location += Rotation.RotateVector(Sockets[i]->RelativeLocation*ControlPoint->MeshScale);
				Rotation += Sockets[i]->RelativeRotation;
				ForwardVector = Sockets[i]->RelativeRotation.RotateVector(ForwardVector);
				ForwardVector.Normalize(); // needs to be normalized after rotation
				ForwardVector *= -1; //Flip Forward for End point
				
				//Add the point with tangent line
				RoadSpline->Spline->AddSplinePoint(Location, ESplineCoordinateSpace::World,true);
				RoadSpline->Spline->SetTangentAtSplinePoint(RoadSpline->Spline->GetNumberOfSplinePoints()-1,
					ForwardVector*TangentSize,
					ESplineCoordinateSpace::World);

				//Spawn point at j///////////////////////////////////////////////////////

				//Get control point location, rotation, and forward vector (Tangent Line)
				Location = ControlPoint->Location + LandscapeSplineActor->GetActorLocation();
				Rotation = ControlPoint->Rotation + LandscapeSplineActor->GetActorRotation();
				ForwardVector = ControlPoint->Rotation.Vector();
				Location += Rotation.RotateVector(Sockets[j]->RelativeLocation*ControlPoint->MeshScale);
				Rotation += Sockets[j]->RelativeRotation;
				ForwardVector = Sockets[j]->RelativeRotation.RotateVector(ForwardVector);
				ForwardVector.Normalize(); // needs to be normalized after rotation
				
				//Add the point with tangent line
				RoadSpline->Spline->AddSplinePoint(Location, ESplineCoordinateSpace::World,true);
				RoadSpline->Spline->SetTangentAtSplinePoint(RoadSpline->Spline->GetNumberOfSplinePoints()-1,
					ForwardVector*TangentSize,
					ESplineCoordinateSpace::World);
			}
		}
	}
}

void UAA_BlueprintFunctionLibrary_CPP::GenerateRaceSpline(const TArray<AActor*> RoadSplineActors, TSubclassOf<AAA_RaceSpline_CPP> RaceSplineBP)
{
	TArray<AAA_RoadSpline_CPP*> RoadSplines;
	for (const auto RoadSpline : RoadSplineActors)
	{
		RoadSplines.Add(Cast<AAA_RoadSpline_CPP>(RoadSpline));
	}
	if(RoadSplines.Num() <  1)
	{
		UE_LOG(LogTemp,Warning,TEXT("GenerateRaceSpline Failed: Selection was empty"))
		return;
	}
	
	//Create a new spline actor
	FVector SpawnLocation = FVector(RoadSplines[0]->GetActorLocation());
	FRotator SpawnRotation = FRotator(RoadSplines[0]->GetActorRotation());
	AAA_RaceSpline_CPP* RaceSpline = Cast<AAA_RaceSpline_CPP>(RoadSplines[0]->GetWorld()->SpawnActor(RaceSplineBP,&SpawnLocation,&SpawnRotation));

	RaceSpline->Spline->ClearSplinePoints();

#if WITH_EDITOR
	RaceSpline->SetFolderPath("RaceSplines");
#endif

	//Init Variables
	FVector Location;
	FVector ArriveTangent = FVector::Zero();
	FVector LeaveTangent;
	FRotator Rotation;
	FSplinePoint NewPoint;
	int PreviousStart;
	int CurrentStart;
	int NextStart;
	int SplinePointCount = 0;

	//Init calculation of which spline point to start from and which is connected to next spline
	//WARNING: Makes assumption that spline points are less than 1 unit away
	FIntVector A = FIntVector(RoadSplines[0]->Spline->GetLocationAtSplinePoint(0,ESplineCoordinateSpace::World));
	FIntVector B = FIntVector(RoadSplines[1]->Spline->GetLocationAtSplinePoint(0,ESplineCoordinateSpace::World));
	FIntVector C = FIntVector(RoadSplines[1]->Spline->GetLocationAtSplinePoint(1,ESplineCoordinateSpace::World));
	NextStart = A == B || A == C;
	CurrentStart = NextStart;

	//Add Spline Point Loop///////////////////////////////////////////////////////////////////////////////////////////
	for(int i = 0; i < RoadSplines.Num(); i++)
	{
		PreviousStart = CurrentStart;
		CurrentStart = NextStart;

		//Determine which Spline Point is the connected next
		if(i != RoadSplines.Num()-1)
		{
			A = FIntVector(RoadSplines[i]->Spline->GetLocationAtSplinePoint(!CurrentStart,ESplineCoordinateSpace::World));
			B = FIntVector(RoadSplines[i+1]->Spline->GetLocationAtSplinePoint(1,ESplineCoordinateSpace::World));
			NextStart = A==B;
		}

		//Get Location 
		RoadSplines[i]->Spline->GetLocationAndTangentAtSplinePoint(CurrentStart,Location,LeaveTangent,ESplineCoordinateSpace::World);
		Rotation = RoadSplines[i]->Spline->GetRotationAtSplinePoint(CurrentStart,ESplineCoordinateSpace::World);
		
		//Flip Tangents if needed
		if(CurrentStart){LeaveTangent *= -1;}
		if(PreviousStart){ArriveTangent *= -1;}
		
		//Add New Point
		NewPoint = FSplinePoint(
			static_cast<float>(SplinePointCount),
			RaceSpline->ActorToWorld().Inverse().TransformPosition(Location),
			RaceSpline->ActorToWorld().Inverse().TransformVector(ArriveTangent),
			RaceSpline->ActorToWorld().Inverse().TransformVector(LeaveTangent),
			RaceSpline->ActorToWorld().Inverse().TransformRotation(Rotation.Quaternion()).Rotator());
		
		RaceSpline->Spline->AddPoint(NewPoint);
		++SplinePointCount;
		ArriveTangent = RoadSplines[i]->Spline->GetTangentAtSplinePoint(!CurrentStart,ESplineCoordinateSpace::World);
	}
	
	//Add Last Point//////////////////////////////////////////////////////////////////////////////////////////////////
	int LastIndex = RoadSplines.Num()-1;
	RoadSplines[LastIndex]->Spline->GetLocationAndTangentAtSplinePoint(!CurrentStart,Location,LeaveTangent,ESplineCoordinateSpace::World);
	Rotation = RoadSplines[LastIndex]->Spline->GetRotationAtSplinePoint(!CurrentStart,ESplineCoordinateSpace::World);
	
	if(CurrentStart){ArriveTangent *= -1;}

	//Add New Point
	NewPoint = FSplinePoint(
	static_cast<float>(SplinePointCount),
	RaceSpline->ActorToWorld().Inverse().TransformPosition(Location),
	RaceSpline->ActorToWorld().Inverse().TransformVector(ArriveTangent),
	RaceSpline->ActorToWorld().Inverse().TransformVector(LeaveTangent),
	RaceSpline->ActorToWorld().Inverse().TransformRotation(Rotation.Quaternion()).Rotator());
	RaceSpline->Spline->AddPoint(NewPoint);
}