// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/AA_RoadSplineActor.h"
#include "Components/SplineComponent.h"

// Sets default values
AAA_RoadSplineActor::AAA_RoadSplineActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Spline = CreateDefaultSubobject<USplineComponent>("RoadSpline");
	SetRootComponent(Spline);
}


