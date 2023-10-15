// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AA_RacerObstacleAvoidanceComponent.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/AlpineAsphaltLogger.h"
#include "Logging/LoggingUtils.h"
#include "AI/AA_RacerContextProvider.h"
#include "Pawn/AA_WheeledVehiclePawn.h"
#include "Util/UnitConversions.h"

using namespace AA;

struct UAA_RacerObstacleAvoidanceComponent::FThreatContext
{
	FVector ReferencePosition;
	FVector ToMovementTargetNormalized;
	const FAA_AIRacerContext* RacerContext;
	float DistanceToTarget;
	float VehicleSpeed;
};

// Sets default values for this component's properties
UAA_RacerObstacleAvoidanceComponent::UAA_RacerObstacleAvoidanceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAA_RacerObstacleAvoidanceComponent::OnVehicleObstaclesUpdated(AAA_WheeledVehiclePawn* VehiclePawn, const TArray<AAA_WheeledVehiclePawn*>& VehicleObstacles)
{
	if (VehicleObstacles.IsEmpty())
	{
		return;
	}

	FThreatContext ThreatContext;
	if (!PopulateThreatContext(ThreatContext))
	{
		return;
	}

	if (VehicleObstacles.IsEmpty())
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
			TEXT("%s-%s: OnVehicleObstaclesUpdated - No detections"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));

		OnVehicleAvoidancePositionUpdated.Broadcast(ThreatContext.RacerContext->VehiclePawn, FAA_AIRacerAvoidanceContext{});
		return;
	}

	FVector AccumulatedThreatVector{ 0 };
	int ThreatCount{};

	for (auto DetectedVehicle : VehicleObstacles)
	{
		if (!DetectedVehicle)
		{
			continue;
		}

		if (const auto ThreatVectorOptional = ComputeThreatVector(ThreatContext, *DetectedVehicle))
		{
			++ThreatCount;
			AccumulatedThreatVector += *ThreatVectorOptional;
		}
	}

	FAA_AIRacerAvoidanceContext AvoidanceContext;
	AvoidanceContext.ThreatCount = ThreatCount;
	AvoidanceContext.ThreatVector = AccumulatedThreatVector.GetSafeNormal();
	AvoidanceContext.NormalizedThreatScore = FMath::Min(AccumulatedThreatVector.Size(), 1.0f);

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
		TEXT("%s-%s: OnVehicleObstaclesUpdated - %s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *AvoidanceContext.ToString());

	OnVehicleAvoidancePositionUpdated.Broadcast(ThreatContext.RacerContext->VehiclePawn, AvoidanceContext);
}

// Called when the game starts
void UAA_RacerObstacleAvoidanceComponent::BeginPlay()
{
	Super::BeginPlay();

	RacerContextProvider = Cast<IAA_RacerContextProvider>(GetOwner());

	if (!RacerContextProvider)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Error,
			TEXT("%s-%s: BeginPlay - Owner does not implement IAA_RacerContextProvider"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
		return;
	}
}

bool UAA_RacerObstacleAvoidanceComponent::PopulateThreatContext(FThreatContext& ThreatContext) const
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

	ThreatContext.RacerContext = &Context;
	ThreatContext.ReferencePosition = MyVehicle->GetFrontWorldLocation();

	const auto& ToMovementTarget = Context.MovementTarget - ThreatContext.ReferencePosition;
	ThreatContext.ToMovementTargetNormalized = ToMovementTarget.GetSafeNormal();
	ThreatContext.DistanceToTarget = ToMovementTarget.Size();

	if (FMath::IsNearlyZero(ThreatContext.DistanceToTarget))
	{
		return false;
	}

	ThreatContext.VehicleSpeed = MyVehicle->GetVehicleSpeed();

	return true;
}

std::optional<FVector> UAA_RacerObstacleAvoidanceComponent::ComputeThreatVector(const FThreatContext& ThreatContext, const AAA_WheeledVehiclePawn& CandidateVehicle) const
{
	const auto& MyReferencePosition = ThreatContext.ReferencePosition;
	const auto& CandidateReferencePosition = CandidateVehicle.GetBackWorldLocation();
	const auto& ToThreat = CandidateReferencePosition - MyReferencePosition;
	const auto CandidateVehicleSpeed = CandidateVehicle.GetVehicleSpeed();

	if (ToThreat.IsNearlyZero())
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
			TEXT("%s-%s: ComputeThreatVector - FALSE - %s ToThreat nearly zero"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *CandidateVehicle.GetName());

		return std::nullopt;
	}

	const auto InterceptTime = (ThreatContext.VehicleSpeed - CandidateVehicleSpeed) / ToThreat.Size();

	if(InterceptTime < 0)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
			TEXT("%s-%s: ComputeThreatVector - FALSE - %s: InterceptTime=%fs < 0; CandidateVehicleSpeed=%fmph"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *CandidateVehicle.GetName(), InterceptTime, CandidateVehicleSpeed * UnitConversions::CmsToMph);

		return std::nullopt;
	}

	const auto CandidateDistanceOverTime = FMath::Abs(CandidateVehicleSpeed) * InterceptTime;

	// Will not catch up to target in time
	if (CandidateDistanceOverTime >= ThreatContext.DistanceToTarget)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
			TEXT("%s-%s: ComputeThreatVector - FALSE - %s: CandidateDistanceOverTime=%fcm >= TargetDistance=%fcm; InterceptTime=%fs,  CandidateVehicleSpeed=%fmph"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *CandidateVehicle.GetName(), CandidateDistanceOverTime, ThreatContext.DistanceToTarget,
			InterceptTime, CandidateVehicleSpeed * UnitConversions::CmsToMph);

		return std::nullopt;
	}

	// Normalize so that closer targets have higher weight
	const auto Score = FMath::Square((ThreatContext.DistanceToTarget - CandidateDistanceOverTime) / ThreatContext.DistanceToTarget);

	return ToThreat.GetSafeNormal() * Score;
}

#if ENABLE_VISUAL_LOG

void UAA_RacerObstacleAvoidanceComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("Racer Obstacle Avoidance Component");

	Snapshot->Status.Add(Category);
}

#endif
