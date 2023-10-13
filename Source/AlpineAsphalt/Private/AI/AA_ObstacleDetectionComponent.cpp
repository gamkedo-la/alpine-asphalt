// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AA_ObstacleDetectionComponent.h"

UAA_ObstacleDetectionComponent::UAA_ObstacleDetectionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickInterval = 0.5f;
}


void UAA_ObstacleDetectionComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


void UAA_ObstacleDetectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

