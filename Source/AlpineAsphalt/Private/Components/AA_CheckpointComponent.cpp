// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/AA_CheckpointComponent.h"

#include "Actors/AA_Checkpoint.h"
#include "..\..\Public\Actors\AA_TrackInfoActor.h"
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
	const auto RaceActor = Cast<AAA_TrackInfoActor>(GetOwner());
	ensure(SplineComponent);

	const int NumberCheckpoints = FMath::CeilToInt(SplineComponent->GetSplineLength() / CheckpointGenerationDistance);

	for(int i = 0; i <= NumberCheckpoints; i++)
	{
		const FVector Position = SplineComponent->GetLocationAtDistanceAlongSpline(i*CheckpointGenerationDistance,ESplineCoordinateSpace::World);
		const FRotator Rotation  = SplineComponent->GetRotationAtDistanceAlongSpline(i*CheckpointGenerationDistance,ESplineCoordinateSpace::World);
		const float Width = RaceActor->GetWidthAtDistance(i*CheckpointGenerationDistance)*2;
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
		NewCheckpoint->SetIndex(i);

		SpawnedCheckpoints.Add(NewCheckpoint);
	}
}

void UAA_CheckpointComponent::DestroyCheckpoints()
{
	for (auto Checkpoint : SpawnedCheckpoints)
	{
		Checkpoint->Destroy();
	}
	SpawnedCheckpoints.Empty();
}

void UAA_CheckpointComponent::ShowRaceFinish(bool bShow)
{
	if (SpawnedCheckpoints.IsEmpty())
	{
		return;
	}

	// TODO: We might also want to have a different asset visible for non-finish checkpoints
	// Also may not want to show the checkered flag at the end of a lap - just at end of race
	SpawnedCheckpoints.Last()->SetIndicatorVisibleInMap(true);
}

float UAA_CheckpointComponent::GetDistanceAlongSplineAtRaceFinish() const
{
	if (!SplineComponent)
	{
		return 0.0f;
	}

	if (SpawnedCheckpoints.IsEmpty())
	{
		return SplineComponent->GetSplineLength();
	}

	auto FinalCheckpoint = SpawnedCheckpoints.Last();
	const FVector RaceFinishWorldLocation = FinalCheckpoint->GetActorLocation();

	const auto SplineKey = SplineComponent->FindInputKeyClosestToWorldLocation(RaceFinishWorldLocation);
	return SplineComponent->GetDistanceAlongSplineAtSplineInputKey(SplineKey);
}


