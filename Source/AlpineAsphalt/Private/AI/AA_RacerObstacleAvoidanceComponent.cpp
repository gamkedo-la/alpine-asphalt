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
	FVector ForwardVector;
	const FAA_AIRacerContext* RacerContext;
	const AAA_WheeledVehiclePawn* VehiclePawn;
	float MinThreatSpeedConsiderationDistance;
	float CarLength;
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
	int32 ThreatCount{};
	float NormalizedSpeed{};
	float ScoreSum{};
	bool bNearThreat{};

	for (auto DetectedVehicle : VehicleObstacles)
	{
		if (!DetectedVehicle)
		{
			continue;
		}

		if (const auto ThreatResultOptional = ComputeThreatResult(ThreatContext, *DetectedVehicle))
		{
			++ThreatCount;
			if (ThreatResultOptional->bIsNearThreat)
			{
				bNearThreat = true;
			}

			AccumulatedThreatVector += ThreatResultOptional->ThreatVector;
			NormalizedSpeed += FMath::Abs(DetectedVehicle->GetVehicleSpeed()) * ThreatResultOptional->Score;
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
	AvoidanceContext.bNearThreat = bNearThreat && ThreatCount == 1;

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

	const auto World = GetWorld();
	if (!World)
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

	if (ThreatContext.ToMovementTargetNormalized.IsNearlyZero())
	{
		return false;
	}

	ThreatContext.ForwardVector = MyVehicle->GetActorForwardVector();
	ThreatContext.CarLength = MyVehicle->GetVehicleLength();
	ThreatContext.MinThreatSpeedConsiderationDistance = MinThreatSpeedCarLengthsDistance * ThreatContext.CarLength;

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
	const bool bIsNearThreat = DistanceToThreat < ThreatContext.MinThreatSpeedConsiderationDistance;
	const auto CandidateVehicleSpeed = GetAverageSpeed(ThreatContext, CandidateVehicle, DistanceToThreat);
	const auto MyVehicleSpeed = GetAverageSpeed(ThreatContext, *ThreatContext.VehiclePawn, DistanceToThreat);

	const auto InterceptTime = DistanceToThreat / FMath::Max(MyVehicleSpeed - CandidateVehicleSpeed, 0.01f);

	bool bWillIntercept = InterceptTime >= 0 && InterceptTime < MaxInterceptTime;

	if(!bIsNearThreat && !bWillIntercept)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
			TEXT("%s-%s: ComputeThreatVector - FALSE - %s: InterceptTime=%fs < 0 || >=%fs; MyVehicleSpeed=%fmph; CandidateVehicleSpeed=%fmph"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *CandidateVehicle.GetName(), InterceptTime, MaxInterceptTime, MyVehicleSpeed * UnitConversions::CmsToMph, CandidateVehicleSpeed * UnitConversions::CmsToMph);

		return std::nullopt;
	}

	// Will not catch up to target in time
	if (!bIsNearThreat && !bWillIntercept)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
			TEXT("%s-%s: ComputeThreatVector - FALSE - %s: DistanceToThreat=%fm; InterceptTime=%fs; MyVehicleSpeed=%fmph; CandidateVehicleSpeed=%fmph"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *CandidateVehicle.GetName(), DistanceToThreat / 100,
			InterceptTime, MyVehicleSpeed * UnitConversions::CmsToMph, CandidateVehicleSpeed * UnitConversions::CmsToMph);

		return std::nullopt;
	}

	
	const auto ToThreatNormalized = ToThreat / DistanceToThreat;
	const auto ThreatAlignmentDotProduct = ToThreatNormalized | ThreatContext.ToMovementTargetNormalized;
	float Score{};

	if (bWillIntercept)
	{
		// Normalize so that closer targets have higher weight and those more aligned to our movement direction
		Score = FMath::Square((MaxInterceptTime - InterceptTime) / MaxInterceptTime) * FMath::Abs(ThreatAlignmentDotProduct);
	}
	if (bIsNearThreat)
	{
		// For near threat compute the alignment based on forward vector
		const auto NearThreatDotProduct = FVector2D(ToThreatNormalized) | FVector2D(ThreatContext.ForwardVector);
		Score = FMath::Max(Score, (ThreatContext.MinThreatSpeedConsiderationDistance - DistanceToThreat) / ThreatContext.MinThreatSpeedConsiderationDistance * FMath::Sqrt(FMath::Abs(NearThreatDotProduct)));
	}

	if (FMath::IsNearlyZero(Score))
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
			TEXT("%s-%s: ComputeThreatVector - FALSE - %s: Score is 0: bIsNearThreat=%s; bWillIntercept=%s; DistanceToThreat=%fm; MinThreatSpeedConsiderationDistance=%fm; InterceptTime=%fs; MyVehicleSpeed=%fmph; CandidateVehicleSpeed=%fmph"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *CandidateVehicle.GetName(), LoggingUtils::GetBoolString(bIsNearThreat), LoggingUtils::GetBoolString(bWillIntercept), DistanceToThreat / 100,
			ThreatContext.MinThreatSpeedConsiderationDistance / 100, InterceptTime, MyVehicleSpeed * UnitConversions::CmsToMph, CandidateVehicleSpeed * UnitConversions::CmsToMph);

		return std::nullopt;
	}
			
	const auto ScaledThreatVector = ToThreatNormalized * Score;

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
		TEXT("%s-%s: ComputeThreatVector - TRUE - %s: Score=%f; bIsNearThreat=%s; bWillIntercept=%s; MyVehicleSpeed=%fmph; CandidateVehicleSpeed=%fmph; InterceptTime=%fs; DistanceToThreat=%fm; MinThreatSpeedConsiderationDistance=%fm; ThreatAlignmentDotProduct=%f; ScaledThreatVector=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *CandidateVehicle.GetName(),
		Score, LoggingUtils::GetBoolString(bIsNearThreat), LoggingUtils::GetBoolString(bWillIntercept), MyVehicleSpeed * UnitConversions::CmsToMph, CandidateVehicleSpeed * UnitConversions::CmsToMph,
		InterceptTime, DistanceToThreat / 100, ThreatContext.MinThreatSpeedConsiderationDistance / 100, ThreatAlignmentDotProduct, *ScaledThreatVector.ToCompactString()
	);

	UE_VLOG_CYLINDER(GetOwner(), LogAlpineAsphalt, Log, CandidateReferencePosition, CandidateReferencePosition + FVector(0, 0, 200.0f), 50.0f, FColor::Red,
		TEXT("%s - Avoidance Threat; Score=%f"), *CandidateVehicle.GetName(), Score);

	return FThreatResult
	{
		.ThreatVector = ScaledThreatVector,
		.Score = Score,
		.bIsNearThreat = bIsNearThreat && Score >= 0.5f
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
