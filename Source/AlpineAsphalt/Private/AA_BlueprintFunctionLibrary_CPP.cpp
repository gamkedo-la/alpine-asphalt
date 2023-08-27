// Fill out your copyright notice in the Description page of Project Settings.


#include "AA_BlueprintFunctionLibrary_CPP.h"
#include "AA_RoadSpline_CPP.h"
#include "HairStrandsInterface.h"
#include "LandscapeSplineActor.h"
#include "LandscapeSplineControlPoint.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Engine/StaticMeshSocket.h"

/**
 * @brief
 * Generates AA_RoadSplines_CPP for the given Landscape Spline
 * @param LandscapeSpline 
 */
void UAA_BlueprintFunctionLibrary_CPP::GenerateRoadSpline(AActor* LandscapeSpline)
{
	//Cast Actor to Landscape Spline Actor
	const ALandscapeSplineActor* LandscapeSplineActor = Cast<ALandscapeSplineActor>(LandscapeSpline);
	check(LandscapeSplineActor);
	
	//Cache World to spawn objects later
	UWorld* World = LandscapeSplineActor->GetWorld();

	TArray<TObjectPtr<ULandscapeSplineSegment>> Segments = LandscapeSplineActor->GetSplinesComponent()->GetSegments();
	
	for (const TObjectPtr<ULandscapeSplineSegment> Segment : Segments)
	{
		//Create a new spline actor
		AAA_RoadSpline_CPP* RoadSpline = World->SpawnActor<AAA_RoadSpline_CPP>();
		RoadSpline->Spline->ClearSplinePoints();
	
#if WITH_EDITOR
		RoadSpline->SetFolderPath("RoadSplines");
#endif
		
		for(int i = 0; i < 2; i++) //There are always two connections, Start and End
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
				AAA_RoadSpline_CPP* RoadSpline = World->SpawnActor<AAA_RoadSpline_CPP>();
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

void UAA_BlueprintFunctionLibrary_CPP::GenerateRaceSpline(TArray<AAA_RoadSpline_CPP*> RoadSplines)
{
	
}
