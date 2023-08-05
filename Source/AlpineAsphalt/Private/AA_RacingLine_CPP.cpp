// Fill out your copyright notice in the Description page of Project Settings.


#include "AA_RacingLine_CPP.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Kismet/KismetMathLibrary.h"

AAA_RacingLine_CPP::AAA_RacingLine_CPP()
{
	RacingLineSpline = CreateDefaultSubobject<USplineComponent>("RacingLineSpline");
	InstancedArrowMeshes = CreateDefaultSubobject<UInstancedStaticMeshComponent>("InstancedArrowMeshes");
}

// Called when the game starts or when spawned
void AAA_RacingLine_CPP::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAA_RacingLine_CPP::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AAA_RacingLine_CPP::SetArrowMeshes()
{
	ensure(RacingLineSpline);
	ensure(InstancedArrowMeshes);
	const int NumberOfArrowMeshesToSpawn = RacingLineSpline->GetSplineLength() / DistanceBetweenArrows;
	for(int i = 0; i < NumberOfArrowMeshesToSpawn; i++)
	{
		//Get Location and rotation along spline
		FQuat Rotation = RacingLineSpline->GetQuaternionAtDistanceAlongSpline(
			i * DistanceBetweenArrows, ESplineCoordinateSpace::World);
		FVector Location = RacingLineSpline->GetLocationAtDistanceAlongSpline(
			i * DistanceBetweenArrows, ESplineCoordinateSpace::World);
		//Spawn mesh with rotation and location
		InstancedArrowMeshes->AddInstance(FTransform(Rotation,Location),true);
	}
}

