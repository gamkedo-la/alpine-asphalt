// Fill out your copyright notice in the Description page of Project Settings.


#include "AA_RaceSpline_CPP.h"

#include "Components/SplineComponent.h"

// Sets default values
AAA_RaceSpline_CPP::AAA_RaceSpline_CPP()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	Spline = CreateDefaultSubobject<USplineComponent>("RacingLineSpline");
	SetRootComponent(Spline);
}

// Called when the game starts or when spawned
void AAA_RaceSpline_CPP::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AAA_RaceSpline_CPP::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

