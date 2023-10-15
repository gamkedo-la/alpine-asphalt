// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AA_ObstacleDetectionComponent.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/AlpineAsphaltLogger.h"
#include "Logging/LoggingUtils.h"
#include "AI/AA_RacerContextProvider.h"
#include "Pawn/AA_WheeledVehiclePawn.h"

using namespace AA;

struct UAA_ObstacleDetectionComponent::FThreatContext
{
	FVector ReferencePosition;
	const FAA_AIRacerContext* RacerContext;
	FVector ToMovementTarget;
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
}

void UAA_ObstacleDetectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!RacerContextProvider)
	{
		return;
	}

	DetectedVehicles.Reset();

	if (AllVehicles.IsEmpty())
	{
		PopulateAllVehicles();
	}

	if (AllVehicles.IsEmpty())
	{
		return;
	}

	FThreatContext ThreatContext;
	PopulateThreatContext(ThreatContext);

	for (auto CandidateVehicle : AllVehicles)
	{
		if (!CandidateVehicle)
		{
			continue;
		}

		if(IsPotentialThreat(ThreatContext, *CandidateVehicle))
		{
			UE_VLOG_CYLINDER(GetOwner(), LogAlpineAsphalt, Verbose, CandidateVehicle->GetActorLocation(), CandidateVehicle->GetActorLocation() + FVector(0, 0, 100.f), 50.0f, FColor::Orange,
				TEXT("%s - Candidate threat: %s"), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(CandidateVehicle));

			DetectedVehicles.Add(CandidateVehicle);
		}
	}

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
		TEXT("%s-%s: TickComponent - Found %d candidate threats"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), DetectedVehicles.Num());

	OnVehicleObstaclesUpdated.Broadcast(RacerContextProvider->GetRacerContext().VehiclePawn, DetectedVehicles);
}

void UAA_ObstacleDetectionComponent::PopulateThreatContext(FThreatContext& ThreatContext) const
{
	check(RacerContextProvider);
	const auto& Context = RacerContextProvider->GetRacerContext();
	const auto MyVehicle = Context.VehiclePawn;
	check(MyVehicle);

	ThreatContext.RacerContext = &Context;
	ThreatContext.ReferencePosition = MyVehicle->GetBackWorldLocation();
	ThreatContext.ToMovementTarget = Context.MovementTarget - ThreatContext.ReferencePosition;
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

bool UAA_ObstacleDetectionComponent::IsPotentialThreat(const FThreatContext& ThreatContext, const AAA_WheeledVehiclePawn& CandidateVehicle) const
{
	// Right now only consider those along side vehicle or in front
	const auto& MyReferencePosition = ThreatContext.ReferencePosition;
	const auto& CandidateReferencePosition = CandidateVehicle.GetFrontWorldLocation();
	const auto ToCandidate = CandidateReferencePosition - MyReferencePosition;

	const auto DotProduct = ToCandidate | ThreatContext.ToMovementTarget;

	if (DotProduct < 0)
	{
		return false;
	}

	return ToCandidate.SizeSquared() <= FMath::Square(MaxDistanceThresholdMeters * 100);
}

#if ENABLE_VISUAL_LOG

void UAA_ObstacleDetectionComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("AI Obstacle Detection");

	Snapshot->Status.Add(Category);
}

#endif

