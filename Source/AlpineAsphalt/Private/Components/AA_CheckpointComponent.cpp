// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/AA_CheckpointComponent.h"

#include "Actors/AA_Checkpoint.h"
#include "Actors/AA_RaceActor.h"
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
	const auto RaceActor = Cast<AAA_RaceActor>(GetOwner());
	ensure(SplineComponent);

	const int NumberCheckpoints = FMath::CeilToInt(SplineComponent->GetSplineLength() / CheckpointGenerationDistance);

	for(int i = 0; i <= NumberCheckpoints; i++)
	{
		const FVector Position = SplineComponent->GetLocationAtDistanceAlongSpline(i*CheckpointGenerationDistance,ESplineCoordinateSpace::World);
		const FRotator Rotation  = SplineComponent->GetRotationAtDistanceAlongSpline(i*CheckpointGenerationDistance,ESplineCoordinateSpace::World);
		const float Width = RaceActor->GetWidthAtDistance(i*CheckpointGenerationDistance);
		CheckpointPositionData.Add(FCheckpointStruct(Position,Rotation,Width,DEFAULT_CHECKPOINT_HEIGHT));
	}
}

USplineComponent* UAA_CheckpointComponent::GetSpline() const
{
	return GetOwner()->GetComponentByClass<USplineComponent>();
}

void UAA_CheckpointComponent::SpawnCheckpoints()
{
	if(!DefaultCheckpoint)
	{
		UE_LOG(LogTemp,Error,TEXT("UAA_CheckpointComponent: NO DEFAULT CHECKPOINT CLASS SET"))
		return;
	}
	if(!SpawnedCheckpoints.IsEmpty())
	{
		UE_LOG(LogTemp,Error,TEXT("UAA_CheckpointComponent: Spawned Checkpoints Already Exist"))
		return;
	}
	for(int i = 0; i < CheckpointPositionData.Num(); i++)
	{
		AAA_Checkpoint* NewCheckpoint = GetWorld()->SpawnActor<AAA_Checkpoint>(DefaultCheckpoint,
			CheckpointPositionData[i].Position,
			CheckpointPositionData[i].Rotation);
		NewCheckpoint->SetWidth(CheckpointPositionData[i].Width);
		NewCheckpoint->SetHeight(CheckpointPositionData[i].Height);
		NewCheckpoint->SetDepth(CheckpointPositionData[i].Depth);
		SpawnedCheckpoints.Add(NewCheckpoint);
	}
}


