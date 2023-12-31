// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AA_ObstacleDetectionComponent.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/AlpineAsphaltLogger.h"
#include "Logging/LoggingUtils.h"
#include "AI/AA_RacerContextProvider.h"
#include "Pawn/AA_WheeledVehiclePawn.h"
#include "Components/SplineComponent.h"
#include "Actors/AA_TrackInfoActor.h"

using namespace AA;

struct UAA_ObstacleDetectionComponent::FThreatContext
{
	FVector ReferencePosition;
	FVector BackReferencePosition;
	const FAA_AIRacerContext* RacerContext;
	AController* MyController;
	FVector MovementDirection;
	float DistanceAlongSpline;
};

UAA_ObstacleDetectionComponent::UAA_ObstacleDetectionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.5f;
}

void UAA_ObstacleDetectionComponent::BeginPlay()
{
	Super::BeginPlay();

	// TODO: Extract this into a base component class implemented by all the Racer AI components
	RacerContextProvider = Cast<IAA_RacerContextProvider>(GetOwner());

	if (!RacerContextProvider)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Error,
			TEXT("%s-%s: BeginPlay - Owner does not implement IAA_RacerContextProvider"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));

		SetComponentTickEnabled(false);
		return;
	}

	RegisterRewindCallback();
}

void UAA_ObstacleDetectionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UnregisterRewindCallback();
}

void UAA_ObstacleDetectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	DoTick();
}

void UAA_ObstacleDetectionComponent::RecalculateOnRewind()
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log, TEXT("%s-%s: RecalculateOnRewind"),
		*GetName(), *LoggingUtils::GetName(GetOwner()));

	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::DoTick);
}

bool UAA_ObstacleDetectionComponent::PopulateThreatContext(FThreatContext& ThreatContext) const
{
	if (!RacerContextProvider)
	{
		return false;
	}

	const auto& Context = RacerContextProvider->GetRacerContext();
	const auto MyVehicle = Context.VehiclePawn;

	if (!MyVehicle)
	{
		return false;
	}

	const auto RaceTrack = Context.RaceTrack;
	if (!RaceTrack)
	{
		return false;
	}

	const auto Spline = RaceTrack->Spline;
	if (!Spline)
	{
		return false;
	}

	ThreatContext.MyController = MyVehicle->GetController();

	if (!ThreatContext.MyController)
	{
		return false;
	}

	ThreatContext.RacerContext = &Context;
	ThreatContext.ReferencePosition = MyVehicle->GetFrontWorldLocation();
	ThreatContext.BackReferencePosition = MyVehicle->GetBackWorldLocation();

	const auto& ForwardVector = MyVehicle->GetActorForwardVector();
	if (MyVehicle->IsReversing())
	{
		ThreatContext.MovementDirection = -ForwardVector;
	}
	else
	{
		ThreatContext.MovementDirection = ForwardVector;
	}
	// Use current location since AIContext.DistanceAlongSpline is the target position and not the current
	ThreatContext.DistanceAlongSpline = GetDistanceAlongSplineAtLocation(*Spline, ThreatContext.ReferencePosition);

	return true;
}

void UAA_ObstacleDetectionComponent::DoTick()
{
	if (!RacerContextProvider)
	{
		return;
	}

	DetectedVehicles.Reset();

	if (AllVehicles.IsEmpty())
	{
		PopulateAllVehicles();
	}

	if (!AllVehicles.IsEmpty())
	{
		FThreatContext ThreatContext;
		if (!PopulateThreatContext(ThreatContext))
		{
			return;
		}

		const auto& AIContext = RacerContextProvider->GetRacerContext();

		for (auto CandidateVehicle : AllVehicles)
		{
			if (!CandidateVehicle)
			{
				continue;
			}

			if (IsPotentialThreat(AIContext, ThreatContext, *CandidateVehicle))
			{
				UE_VLOG_CYLINDER(GetOwner(), LogAlpineAsphalt, Verbose, CandidateVehicle->GetActorLocation(), CandidateVehicle->GetActorLocation() + FVector(0, 0, 100.f), 50.0f, FColor::Orange,
					TEXT("%s - Candidate threat: %s"), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(CandidateVehicle));

				DetectedVehicles.Add(CandidateVehicle);
			}
		}
	}

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
		TEXT("%s-%s: TickComponent - Found %d candidate threats"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), DetectedVehicles.Num());

	OnVehicleObstaclesUpdated.Broadcast(RacerContextProvider->GetRacerContext().VehiclePawn, DetectedVehicles);
}

void UAA_ObstacleDetectionComponent::PopulateAllVehicles()
{
	AllVehicles.Reset();

	const auto GameWorld = GetWorld();

	check(RacerContextProvider);
	const auto& Context = RacerContextProvider->GetRacerContext();
	const auto MyVehicle = Context.VehiclePawn;

	if (!MyVehicle)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Warning,
			TEXT("%s-%s: PopulateAllVehicles - AI Racer Context vehicle is NULL - skipping"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), AllVehicles.Num());
		return;
	}

	for (TObjectIterator<AAA_WheeledVehiclePawn> It; It; ++It)
	{
		if (GameWorld == It->GetWorld() && *It != MyVehicle)
		{
			AllVehicles.Add(*It);
		}
	}

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
		TEXT("%s-%s: PopulateAllVehicles - Found %d vehicles in world"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), AllVehicles.Num());
}

bool UAA_ObstacleDetectionComponent::IsPotentialThreat(const FAA_AIRacerContext& AIContext, const FThreatContext& ThreatContext, const AAA_WheeledVehiclePawn& CandidateVehicle) const
{
	// Right now only consider those along side vehicle or in front
	const auto& MyReferencePosition = ThreatContext.ReferencePosition;
	const auto& CandidateReferencePosition = CandidateVehicle.GetBackWorldLocation();
	const auto ToCandidateBack = CandidateReferencePosition - MyReferencePosition;
	const auto ToCandidateFront = CandidateVehicle.GetFrontWorldLocation() - ThreatContext.BackReferencePosition;

	// Make sure we aren't alongside the vehicle

	const auto DotProductFront = ToCandidateBack | ThreatContext.MovementDirection;
	const auto DotProductBack = ToCandidateFront | ThreatContext.MovementDirection;

	if (DotProductFront < 0 && DotProductBack < 0)
	{
		return false;
	}

	// Quick straight-line distance check
	const auto MaxDistanceThreshold = MaxDistanceThresholdMeters * 100;

	if (ToCandidateBack.SizeSquared() > FMath::Square(MaxDistanceThreshold))
	{
		return false;
	}

	// Check LOS - look a bit above vehicle
	if (!ThreatContext.MyController->LineOfSightTo(&CandidateVehicle, CandidateVehicle.GetTopWorldLocation() + FVector(0, 0, 200), true))
	{
		return false;
	}

	const auto MyDistanceAlongSpine = ThreatContext.DistanceAlongSpline;
	if (auto RaceTrackSpline = AIContext.RaceTrack ? AIContext.RaceTrack->Spline : nullptr; RaceTrackSpline)
	{
		// This is an expensive call but we don't tick at a high rate.  
		// We could use a cached value, but it is more accurate if we use the current position and here we want to use the back location and not front.
		const auto CandidateDistanceAlongSpline = GetDistanceAlongSplineAtLocation(*RaceTrackSpline, CandidateReferencePosition);

		if (FMath::Abs(CandidateDistanceAlongSpline - MyDistanceAlongSpine) > MaxDistanceThreshold)
		{
			return false;
		}
	}

	return true;
}

float UAA_ObstacleDetectionComponent::GetDistanceAlongSplineAtLocation(const USplineComponent& SplineComponent, const FVector& WorldLocation)
{
	const auto Key = SplineComponent.FindInputKeyClosestToWorldLocation(WorldLocation);
	return SplineComponent.GetDistanceAlongSplineAtSplineInputKey(Key);
}

#if ENABLE_VISUAL_LOG

void UAA_ObstacleDetectionComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("AI Obstacle Detection");

	Snapshot->Status.Add(Category);
}

#endif

