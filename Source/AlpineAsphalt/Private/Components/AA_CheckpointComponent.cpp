// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/AA_CheckpointComponent.h"
#include "Components/SplineComponent.h"

// Sets default values for this component's properties
UAA_CheckpointComponent::UAA_CheckpointComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	
}


// Called when the game starts
void UAA_CheckpointComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UAA_CheckpointComponent::InitializeComponent()
{
	Super::InitializeComponent();

	//Owner should have a Spline Component
	SplineComponent = GetOwner()->GetComponentByClass<USplineComponent>();
	if(!SplineComponent)
	{
		UE_LOG(LogTemp,Error,TEXT("CheckpointComponent could not find Spline Component on actor."))
	}
}


// Called every frame
void UAA_CheckpointComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UAA_CheckpointComponent::GenerateCheckpoints()
{
	SplineComponent = GetOwner()->GetComponentByClass<USplineComponent>();

	ensure(SplineComponent);

	const int NumberCheckpoints = FMath::CeilToInt(SplineComponent->GetSplineLength() / CheckpointGenerationDistance);

	for(int i = 0; i <= NumberCheckpoints; i++)
	{
		CheckpointPositionData.Add(FCheckpointStruct(i*CheckpointGenerationDistance));
	}
}

USplineComponent* UAA_CheckpointComponent::GetSpline() const
{
	return GetOwner()->GetComponentByClass<USplineComponent>();
}


