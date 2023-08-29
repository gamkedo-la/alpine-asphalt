// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/AA_RaceSplineActor.h"
#include "Components/SplineComponent.h"

// Sets default values
AAA_RaceSplineActor::AAA_RaceSplineActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	Spline = CreateDefaultSubobject<USplineComponent>("RacingLineSpline");
	SetRootComponent(Spline);
}

// Called when the game starts or when spawned
void AAA_RaceSplineActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAA_RaceSplineActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

