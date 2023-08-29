// Fill out your copyright notice in the Description page of Project Settings.


#include "AA_RoadSpline_CPP.h"

#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"

// Sets default values
AAA_RoadSpline_CPP::AAA_RoadSpline_CPP()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Spline = CreateDefaultSubobject<USplineComponent>("RoadSpline");
	SetRootComponent(Spline);
}

