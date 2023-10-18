// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AA_RacerObstacleAvoidanceComponent.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/AlpineAsphaltLogger.h"
#include "Logging/LoggingUtils.h"
#include "AI/AA_RacerContextProvider.h"
#include "AI/AA_AIRacerContext.h"
#include "Pawn/AA_WheeledVehiclePawn.h"
#include "Util/UnitConversions.h"

using namespace AA;

DEFINE_VLOG_EVENT(EventVehicleThreatsDetected, Display, "Threats Detected")

struct UAA_RacerObstacleAvoidanceComponent::FThreatContext
{
	FVector ReferencePosition;
	FVector ToMovementTargetNormalized;
	const FAA_AIRacerContext* RacerContext;
	const AAA_WheeledVehiclePawn* VehiclePawn;
	float DistanceToTarget;
	float MinThreatSpeedConsiderationDistance;
};

// Sets default values for this component's properties
UAA_RacerObstacleAvoidanceComponent::UAA_RacerObstacleAvoidanceComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAA_RacerObstacleAvoidanceComponent::OnVehicleObstaclesUpdated(AAA_WheeledVehiclePawn* VehiclePawn, const TArray<AAA_WheeledVehiclePawn*>& VehicleObstacles)
{
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
	float NormalizedSpeed{};
	float ScoreSum{};

	for (auto DetectedVehicle : VehicleObstacles)
	{
		if (!DetectedVehicle)
		{
			continue;
		}

		if (const auto ThreatResultOptional = ComputeThreatResult(ThreatContext, *DetectedVehicle))
		{
			++ThreatCount;
			AccumulatedThreatVector += ThreatResultOptional->ThreatVector;
			NormalizedSpeed += DetectedVehicle->GetVehicleSpeed() * ThreatResultOptional->Score;
			ScoreSum += ThreatResultOptional->Score;
		}
	}

	if (ThreatCount > 0)
	{
		NormalizedSpeed /= ScoreSum;

		UE_VLOG_EVENT_WITH_DATA(GetOwner(), EventVehicleThreatsDetected);
	}

	FAA_AIRacerAvoidanceContext AvoidanceContext;
	AvoidanceContext.ThreatCount = ThreatCount;
	AvoidanceContext.ThreatVector = AccumulatedThreatVector.GetSafeNormal();
	AvoidanceContext.NormalizedThreatScore = FMath::Min(AccumulatedThreatVector.Size(), 1.0f);
	AvoidanceContext.NormalizedThreatSpeedMph = NormalizedSpeed * UnitConversions::CmsToMph;

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

	ThreatContext.VehiclePawn = MyVehicle;
	ThreatContext.RacerContext = &Context;
	ThreatContext.ReferencePosition = MyVehicle->GetFrontWorldLocation();

	const auto& ToMovementTarget = Context.MovementTarget - ThreatContext.ReferencePosition;
	ThreatContext.ToMovementTargetNormalized = ToMovementTarget.GetSafeNormal();
	ThreatContext.DistanceToTarget = ToMovementTarget.Size();

	if (FMath::IsNearlyZero(ThreatContext.DistanceToTarget))
	{
		return false;
	}

	ThreatContext.MinThreatSpeedConsiderationDistance = MinThreatSpeedCarLengthsDistance * MyVehicle->GetVehicleLength();

	return true;
}

const FAA_AIRacerContext* UAA_RacerObstacleAvoidanceComponent::GetRacerAIContext(const AAA_WheeledVehiclePawn& CandidateVehicle)
{
	const auto Controller = CandidateVehicle.GetController();
	if (!Controller || Controller->IsPlayerController())
	{
		return nullptr;
	}

	const auto RacerContextProvider = Cast<IAA_RacerContextProvider>(Controller);
	if (!RacerContextProvider)
	{
		return nullptr;
	}

	return &RacerContextProvider->GetRacerContext();
}

bool UAA_RacerObstacleAvoidanceComponent::WillBeAccelerating(const AAA_WheeledVehiclePawn& CandidateVehicle, const FAA_AIRacerContext* AIContext)
{
	if (AIContext)
	{
		return AIContext->DesiredSpeedMph > CandidateVehicle.GetVehicleSpeedMph();
	}

	return CandidateVehicle.IsAccelerating();
}

bool UAA_RacerObstacleAvoidanceComponent::WillBeBraking(const AAA_WheeledVehiclePawn& CandidateVehicle, const FAA_AIRacerContext* AIContext)
{
	if (AIContext)
	{
		return AIContext->DesiredSpeedMph < CandidateVehicle.GetVehicleSpeedMph();
	}

	return CandidateVehicle.IsBraking();
}

float UAA_RacerObstacleAvoidanceComponent::GetAverageSpeed(const FThreatContext& ThreatContext, const AAA_WheeledVehiclePawn& CandidateVehicle, float ThreatDistance) const
{
	const auto* AIContext = GetRacerAIContext(CandidateVehicle);
	const auto InitialVehicleSpeed = CandidateVehicle.GetVehicleSpeed();

	float Acceleration;

	if (WillBeAccelerating(CandidateVehicle, AIContext))
	{
		Acceleration = AverageAcceleration;
	}
	else if (WillBeBraking(CandidateVehicle, AIContext))
	{
		Acceleration = -AverageDeceleration;
	}
	else
	{
		// Just return the current speed
		return InitialVehicleSpeed;
	}

	// V^2 = Vo^2 + 2*a*x
	float RawFinalSpeed = FMath::Sqrt(FMath::Max(0.0f, FMath::Square(InitialVehicleSpeed) + 2 * Acceleration * ThreatDistance));
	float FinalSpeed;

	if (AIContext)
	{
		// Clamp final velocity to target speed
		const auto DesiredSpeed = AIContext->DesiredSpeedMph * UnitConversions::MphToCms;

		if (DesiredSpeed > InitialVehicleSpeed)
		{
			FinalSpeed = FMath::Min(RawFinalSpeed, DesiredSpeed);
		}

		FinalSpeed = FMath::Max(RawFinalSpeed, DesiredSpeed);
	}
	else
	{
		FinalSpeed = RawFinalSpeed;
	}
	
	return (InitialVehicleSpeed + FinalSpeed) * 0.5f;
}

std::optional<UAA_RacerObstacleAvoidanceComponent::FThreatResult> UAA_RacerObstacleAvoidanceComponent::ComputeThreatResult(const FThreatContext& ThreatContext, const AAA_WheeledVehiclePawn& CandidateVehicle) const
{
	const auto& MyReferencePosition = ThreatContext.ReferencePosition;
	const auto& CandidateReferencePosition = CandidateVehicle.GetBackWorldLocation();
	const auto& ToThreat = CandidateReferencePosition - MyReferencePosition;

	if (ToThreat.IsNearlyZero())
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
			TEXT("%s-%s: ComputeThreatVector - FALSE - %s ToThreat nearly zero"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *CandidateVehicle.GetName());

		return std::nullopt;
	}

	const auto DistanceToThreat = ToThreat.Size();
	const bool bIsNearThreat = DistanceToThreat <= ThreatContext.MinThreatSpeedConsiderationDistance;
	const auto CandidateVehicleSpeed = GetAverageSpeed(ThreatContext, CandidateVehicle, DistanceToThreat);
	const auto MyVehicleSpeed = GetAverageSpeed(ThreatContext, *ThreatContext.VehiclePawn, DistanceToThreat);

	const auto InterceptTime = (MyVehicleSpeed - CandidateVehicleSpeed) / DistanceToThreat;

	// TODO: Compiler will probably optimize these since they are const but if this strategy works consider explicitly skipping these calculations that aren't used if bIsNearThreat is true

	if(!bIsNearThreat && InterceptTime < 0)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
			TEXT("%s-%s: ComputeThreatVector - FALSE - %s: InterceptTime=%fs < 0; CandidateVehicleSpeed=%fmph"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *CandidateVehicle.GetName(), InterceptTime, CandidateVehicleSpeed * UnitConversions::CmsToMph);

		return std::nullopt;
	}

	const auto CandidateDistanceOverTime = FMath::Abs(CandidateVehicleSpeed) * InterceptTime;

	// Will not catch up to target in time
	if (!bIsNearThreat && CandidateDistanceOverTime >= ThreatContext.DistanceToTarget)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
			TEXT("%s-%s: ComputeThreatVector - FALSE - %s: CandidateDistanceOverTime=%fm >= TargetDistance=%fm; InterceptTime=%fs,  CandidateVehicleSpeed=%fmph"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *CandidateVehicle.GetName(), CandidateDistanceOverTime / 100, ThreatContext.DistanceToTarget / 100,
			InterceptTime, CandidateVehicleSpeed * UnitConversions::CmsToMph);

		return std::nullopt;
	}

	
	const auto ToThreatNormalized = ToThreat / DistanceToThreat;
	const auto ThreatAlignmentDotProduct = ToThreatNormalized | ThreatContext.ToMovementTargetNormalized;
	float Score;

	if (bIsNearThreat)
	{
		Score = (ThreatContext.DistanceToTarget - DistanceToThreat) / ThreatContext.DistanceToTarget;
	}
	else
	{
		// Normalize so that closer targets have higher weight and those more aligned to our movement direction
		Score = FMath::Square((ThreatContext.DistanceToTarget - CandidateDistanceOverTime) / ThreatContext.DistanceToTarget) * FMath::Sqrt(ThreatAlignmentDotProduct);
	}

	if (FMath::IsNearlyZero(Score))
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
			TEXT("%s-%s: ComputeThreatVector - FALSE - %s: Score is 0: bIsNearThreat=%s; CandidateDistanceOverTime=%fm; TargetDistance=%fm; InterceptTime=%fs,  CandidateVehicleSpeed=%fmph"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *CandidateVehicle.GetName(), LoggingUtils::GetBoolString(bIsNearThreat), CandidateDistanceOverTime / 100, ThreatContext.DistanceToTarget / 100,
			InterceptTime, CandidateVehicleSpeed * UnitConversions::CmsToMph);

		return std::nullopt;
	}
			
	const auto& ToThreatFront = CandidateVehicle.GetFrontWorldLocation() - MyReferencePosition;

	const auto ScaledThreatVector = ToThreatFront.GetSafeNormal() * Score;

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
		TEXT("%s-%s: ComputeThreatVector - TRUE - %s: Score=%f; bIsNearThreat=%s; CandidateVehicleSpeed=%fmph; InterceptTime=%fs; CandidateDistanceOverTime=%fm; TargetDistance=%fm; ThreatAlignmentDotProduct=%f; ScaledThreatVector=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *CandidateVehicle.GetName(),
		Score, LoggingUtils::GetBoolString(bIsNearThreat), CandidateVehicleSpeed * UnitConversions::CmsToMph, InterceptTime, CandidateDistanceOverTime / 100, ThreatContext.DistanceToTarget / 100, ThreatAlignmentDotProduct, *ScaledThreatVector.ToCompactString());

	UE_VLOG_CYLINDER(GetOwner(), LogAlpineAsphalt, Log, CandidateReferencePosition, CandidateReferencePosition + FVector(0, 0, 200.0f), 50.0f, FColor::Red,
		TEXT("%s - Avoidance Threat; Score=%f"), *CandidateVehicle.GetName(), Score);

	return FThreatResult
	{
		.ThreatVector = ScaledThreatVector,
		.Score = Score
	};
}

#if ENABLE_VISUAL_LOG

void UAA_RacerObstacleAvoidanceComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("Racer Obstacle Avoidance Component");

	Snapshot->Status.Add(Category);
}

#endif
