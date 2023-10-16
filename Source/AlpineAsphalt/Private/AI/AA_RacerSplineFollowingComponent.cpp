// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AA_RacerSplineFollowingComponent.h"

#include "AI/AA_AIRacerContext.h"
#include "AI/AA_RacerContextProvider.h"
#include "Components/SplineComponent.h"
#include "Actors/AA_TrackInfoActor.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/AlpineAsphaltLogger.h"
#include "Logging/LoggingUtils.h"

#include "Pawn/AA_WheeledVehiclePawn.h"


using namespace AA;

// Sets default values for this component's properties
UAA_RacerSplineFollowingComponent::UAA_RacerSplineFollowingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAA_RacerSplineFollowingComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
		TEXT("%s-%s: BeginPlay"), *GetName(), *LoggingUtils::GetName(GetOwner()));

	RacerContextProvider = Cast<IAA_RacerContextProvider>(GetOwner());

	if (!RacerContextProvider)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Error,
			TEXT("%s-%s: BeginPlay - Owner does not implement IAA_RacerContextProvider"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
		return;
	}

	LastCurvature = 1.0f;

	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::SetInitialMovementTarget);
}

void UAA_RacerSplineFollowingComponent::SelectNewMovementTarget(AAA_WheeledVehiclePawn* VehiclePawn, const FVector& PreviousMovementTarget)
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log, TEXT("%s-%s: SelectNewMovementTarget: VehiclePawn=%s; PreviousMovementTarget=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(VehiclePawn), *PreviousMovementTarget.ToCompactString());

	check(VehiclePawn);

	check(RacerContextProvider);
	auto& Context = RacerContextProvider->GetRacerContext();

	if (!Context.RaceTrack)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Warning,
			TEXT("%s-%s: SetInitialMovementTarget - RaceTrack not set!"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
		return;
	}
	if (!Context.RaceTrack->Spline)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Error,
			TEXT("%s-%s: SetInitialMovementTarget - RaceTrack=%s does not have a Spline set!"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *Context.RaceTrack->GetName());
		return;
	}

	LastSplineState = GetNextSplineState(Context);

	if (!LastSplineState)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Display,
			TEXT("%s-%s: SetInitialMovementTarget - RaceTrack=%s Completed!"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *Context.RaceTrack->GetName());
		return;
	}

	UpdateMovementFromLastSplineState(Context);
}

void UAA_RacerSplineFollowingComponent::OnVehicleAvoidancePositionUpdated(AAA_WheeledVehiclePawn* VehiclePawn, const FAA_AIRacerAvoidanceContext& AvoidanceContext)
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log, TEXT("%s-%s: OnVehicleAvoidancePositionUpdated: VehiclePawn=%s; AvoidanceContext=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(VehiclePawn), *AvoidanceContext.ToString());

	check(VehiclePawn);

	if (AvoidanceContext.ThreatCount == 0 || FMath::IsNearlyZero(AvoidanceContext.NormalizedThreatScore))
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose, TEXT("%s-%s: OnVehicleAvoidancePositionUpdated: Updated Offset to 0"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));

		ResetAvoidanceContext();
		return;
	}

	if (!RacerContextProvider || !RacerContextProvider->GetRacerContext().RaceTrack || !LastSplineState)
	{
		ResetAvoidanceContext();
		return;
	}

	const auto RaceTrack = RacerContextProvider->GetRacerContext().RaceTrack;
	const auto VehicleHalfWidth = VehiclePawn->GetVehicleWidth() * 0.5f;
	const auto RoadHalfWidth = RaceTrack->GetWidthAtDistance(LastSplineState->DistanceAlongSpline) * 0.5f;
	const auto MaxDelta = RoadHalfWidth - VehicleHalfWidth;

	if (MaxDelta < 0)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Warning, TEXT("%s-%s: OnVehicleAvoidancePositionUpdated: Unable to offset as VehicleHalfWidth=%f > RoadHalfWidth=%f - VehiclePawn=%s; AvoidanceContext=%s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()),
			VehicleHalfWidth, RoadHalfWidth,
			*LoggingUtils::GetName(VehiclePawn), *AvoidanceContext.ToString());

		ResetAvoidanceContext();
		return;
	}

	auto& RacerContext = RacerContextProvider->GetRacerContext();
	const auto& RacerReferencePosition = VehiclePawn->GetFrontWorldLocation();
	const auto MovementVector = (RacerContext.MovementTarget - RacerReferencePosition).GetSafeNormal();

	// parallel choose max delta
	if (FMath::IsNearlyEqual(AvoidanceContext.ThreatVector | MovementVector, 1.0f))
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose, TEXT("%s-%s: OnVehicleAvoidancePositionUpdated: Parallel to movement vector - set CurrentOffset=%f"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), CurrentOffset);

		CurrentOffset = MaxDelta;
	}
	else
	{
		const auto AvoidanceTargetVector = FMath::GetReflectionVector(AvoidanceContext.ThreatVector, MovementVector);
		// cross product of reflection vector to choose which side of road to go on -
		//  Unreal uses Left hand rule since it is a left handed coordinate system so need to invert the order of cross product
		const auto CrossProduct = AvoidanceTargetVector ^ MovementVector;
		CurrentOffset = MaxDelta * FMath::Sign(CrossProduct.Z) * AvoidanceContext.NormalizedThreatScore;
	}

	// Apply Offset immediately
	auto Spline = RacerContext.RaceTrack->Spline;
	check(Spline);

	const auto& ForwardVector = VehiclePawn->GetActorForwardVector();
	for (int32 i = 0; i < 2; ++i)
	{
		const auto& RightVector = Spline->GetRightVectorAtSplineInputKey(LastSplineState->SplineKey, ESplineCoordinateSpace::World);
		const auto NewCandidateLocation = LastSplineState->WorldLocation = LastSplineState->OriginalWorldLocation + RightVector * CurrentOffset;

		const auto ToNewCandidateLocation = NewCandidateLocation - RacerReferencePosition;
		// If the new location is behind us due to highly curved spline, then select a new target and offset again - only due this one additional time
		if ((LastSplineState->WorldLocation | ForwardVector) > 0)
		{
			break;
		}

		LastSplineState = GetNextSplineState(RacerContext);
	}

	UE_VLOG_ARROW(GetOwner(), LogAlpineAsphalt, Log, RacerReferencePosition + FVector(0, 0, 50.0f), RacerReferencePosition + FVector(0, 0, 50.0f) + AvoidanceContext.ThreatVector * LookaheadDistance, FColor::Orange,
		TEXT("%s - ThreatVector - Score=%s"), *VehiclePawn->GetName(), *FString::Printf(TEXT("%.3f"), AvoidanceContext.NormalizedThreatScore));

	UE_VLOG_ARROW(GetOwner(), LogAlpineAsphalt, Log, RacerReferencePosition + FVector(0, 0, 50.0f), RacerContext.MovementTarget + FVector(0,0,50.0f), FColor::Green,
		TEXT("%s - MovementVector"), *VehiclePawn->GetName());

	UE_VLOG_LOCATION(GetOwner(), LogAlpineAsphalt, Log, LastSplineState->WorldLocation + FVector(0,0,50.0f), 100.0f, FColor::Red,
		TEXT("%s - Avoidance Target; CurrentOffset=%f"), *VehiclePawn->GetName(), CurrentOffset);

	LastAvoidanceContext = LastAvoidanceContext;

	UpdateMovementFromLastSplineState(RacerContext);
}

void UAA_RacerSplineFollowingComponent::ResetAvoidanceContext()
{
	CurrentOffset = 0;
	LastAvoidanceContext.reset();
}

void UAA_RacerSplineFollowingComponent::SelectUnstuckTarget(AAA_WheeledVehiclePawn* VehiclePawn, const FVector& IdealSeekPosition)
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log, TEXT("%s-%s: SelectUnstuckTarget: VehiclePawn=%s; IdealSeekPosition=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(VehiclePawn), *IdealSeekPosition.ToCompactString());

	check(VehiclePawn);
	check(RacerContextProvider);
	auto& RacerContext = RacerContextProvider->GetRacerContext();

	check(RacerContext.RaceTrack);
	check(RacerContext.RaceTrack->Spline);

	auto RaceSpline = RacerContext.RaceTrack->Spline;

	const auto Key = RaceSpline->FindInputKeyClosestToWorldLocation(IdealSeekPosition);
	const auto DistanceAlongSpline = RaceSpline->GetDistanceAlongSplineAtSplineInputKey(Key);

	auto NextSplineState = GetNextSplineState(RacerContext, DistanceAlongSpline);

	if (!NextSplineState)
	{
		return;
	}

	LastSplineState = NextSplineState;

	UpdateMovementFromLastSplineState(RacerContext);
}

void UAA_RacerSplineFollowingComponent::SetInitialMovementTarget()
{
	check(RacerContextProvider);
	auto& Context = RacerContextProvider->GetRacerContext();

	LastSplineState = GetInitialSplineState(Context);
	if (!LastSplineState)
	{
		return;
	}

	check(Context.VehiclePawn);

	// This isn't perfect as the road could curve but should be an okay approximation and it's just the start which is usually mostly straight anyway
	const auto CurrentDistance = FVector::Distance(LastSplineState->WorldLocation, Context.VehiclePawn->GetFrontWorldLocation());
	// check the distance to the first point, if it is >= NextDistanceAlongSpline then just return; otherwise increase up to that point
	if (CurrentDistance < LookaheadDistance)
	{
		LastSplineState = GetNextSplineState(Context, LastSplineState->DistanceAlongSpline + LookaheadDistance - CurrentDistance);
		if (!LastSplineState)
		{
			return;
		}
	}

	UpdateMovementFromLastSplineState(Context);
}

std::optional<UAA_RacerSplineFollowingComponent::FSplineState> UAA_RacerSplineFollowingComponent::GetInitialSplineState(const FAA_AIRacerContext& RacerContext) const
{
	check(RacerContext.RaceTrack);
	check(RacerContext.RaceTrack->Spline);
	check(RacerContext.VehiclePawn);

	auto Spline = RacerContext.RaceTrack->Spline;
	auto Vehicle = RacerContext.VehiclePawn;

	FSplineState State;

	const auto& CurrentVehicleLocation = Vehicle->GetFrontWorldLocation();

	const auto Key = State.SplineKey = 0;
	State.DistanceAlongSpline = 0;
	State.SplineDirection = Spline->GetDirectionAtSplineInputKey(Key, ESplineCoordinateSpace::World);
	State.WorldLocation = State.OriginalWorldLocation = Spline->GetWorldLocationAtDistanceAlongSpline(State.DistanceAlongSpline);

	return State;
}

std::optional<UAA_RacerSplineFollowingComponent::FSplineState> UAA_RacerSplineFollowingComponent::GetNextSplineState(const FAA_AIRacerContext& RacerContext, std::optional<float> NextDistanceAlongSplineOverride) const
{
	check(RacerContext.RaceTrack);
	check(RacerContext.RaceTrack->Spline);
	check(RacerContext.VehiclePawn);
	check(LastSplineState);

	auto Spline = RacerContext.RaceTrack->Spline;
	auto Vehicle = RacerContext.VehiclePawn;

	// Adjust lookahead based on current curvature and speed of car
	// We need to actually lookahead further as curvature increases so that we can adjust for the car turning
	// Alpha of 1 is max distance and 0 is min distance lookahead
	const auto LookaheadSpeedAlpha = FMath::Max(0, Vehicle->GetVehicleSpeedMph() - MinSpeedMph) / (MaxSpeedMph - MinSpeedMph);
	const auto LookaheadCurvatureAlpha = LastCurvature;
	const auto LookaheadAlpha = LookaheadCurvatureAlpha * LookaheadCurvatureAlphaWeight + LookaheadSpeedAlpha * (1 - LookaheadCurvatureAlphaWeight);

	const auto CurrentLookaheadDistance = FMath::Lerp(MinLookaheadDistance, LookaheadDistance, LookaheadAlpha);

	const auto NextIdealDistanceAlongSpline = NextDistanceAlongSplineOverride ? *NextDistanceAlongSplineOverride : LastSplineState->DistanceAlongSpline + CurrentLookaheadDistance;

	// If at end of spline, then wrap around
	const auto NextDistanceAlongSpline = FMath::Wrap(NextIdealDistanceAlongSpline, 0.0f, Spline->GetSplineLength());

	FSplineState State;

	const auto Key = State.SplineKey = Spline->GetInputKeyValueAtDistanceAlongSpline(NextDistanceAlongSpline);
	State.DistanceAlongSpline = NextDistanceAlongSpline;
	State.SplineDirection = Spline->GetDirectionAtSplineInputKey(Key, ESplineCoordinateSpace::World);
	State.WorldLocation = State.OriginalWorldLocation = Spline->GetWorldLocationAtDistanceAlongSpline(NextDistanceAlongSpline);

	// Offset road target position
	if (!FMath::IsNearlyZero(CurrentOffset))
	{
		const auto& RightVector = Spline->GetRightVectorAtSplineInputKey(Key, ESplineCoordinateSpace::World);
		State.WorldLocation += RightVector * CurrentOffset;
	}

	return State;
}

void UAA_RacerSplineFollowingComponent::UpdateMovementFromLastSplineState(FAA_AIRacerContext& RacerContext)
{
	check(LastSplineState);

	// Adjust speed based on upcoming curvature
	LastCurvature = CalculateUpcomingRoadCurvature();

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
		TEXT("%s-%s: UpdateMovementFromLastSplineState - Curvature=%f"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), LastCurvature);

	const auto StraightnessFactor = 1 - LastCurvature;

	auto NewSpeed = ClampSpeed(MaxSpeedMph * StraightnessFactor);
	// Scale the speed to avoidance target as well
	if (LastAvoidanceContext && NewSpeed > LastAvoidanceContext->NormalizedThreatSpeedMph)
	{
		const auto InitialNewSpeed = NewSpeed;
		NewSpeed = ClampSpeed(
			NewSpeed * (1 - LastAvoidanceContext->NormalizedThreatScore) + LastAvoidanceContext->NormalizedThreatScore * LastAvoidanceContext->NormalizedThreatSpeedMph);

		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
			TEXT("%s-%s: UpdateMovementFromLastSplineState - Adjusting speed target based on avoidance data: InitialNewSpeed=%fmph; AdjustedNewSpeed=%fmph; AvoidanceContext=%s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), InitialNewSpeed, NewSpeed, *LastAvoidanceContext->ToString());
	}

	RacerContext.DesiredSpeedMph = NewSpeed;
	RacerContext.MovementTarget = LastSplineState->WorldLocation;
	RacerContext.DistanceAlongSpline = LastSplineState->DistanceAlongSpline;

	OnVehicleTargetUpdated.Broadcast(RacerContext.VehiclePawn, RacerContext.MovementTarget, RacerContext.DesiredSpeedMph);
}

float UAA_RacerSplineFollowingComponent::CalculateUpcomingRoadCurvature() const
{
	// A simple first approach is to take the spline direction at the start and compare it to start + lookahead*factor
	if (!LastSplineState || !RacerContextProvider)
	{
		return 0.0f;
	}

	const auto LookaheadState = GetNextSplineState(RacerContextProvider->GetRacerContext(), LastSplineState->DistanceAlongSpline + LookaheadDistance * RoadCurvatureLookaheadFactor);

	if (!LookaheadState)
	{
		return 0.0f;
	}

	// [-1,1]
	const auto DotProduct = LastSplineState->SplineDirection | LookaheadState->SplineDirection;

	// If DotProduct is 1 then return 0, if -1 then return 1
	return (1 - DotProduct) * 0.5f;
}

#if ENABLE_VISUAL_LOG

void UAA_RacerSplineFollowingComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("Racer Spline Following Component");
	
	Category.Add(TEXT("LastCurvature"), FString::Printf(TEXT("%.1f"), LastCurvature));
	Category.Add(TEXT("CurrentOffset"), FString::Printf(TEXT("%.1f"), CurrentOffset));

	if (RacerContextProvider && LastSplineState && RacerContextProvider->GetRacerContext().VehiclePawn && 
		RacerContextProvider->GetRacerContext().RaceTrack && RacerContextProvider->GetRacerContext().RaceTrack->Spline)
	{
		const auto& Context = RacerContextProvider->GetRacerContext();
		const auto Spline = Context.RaceTrack->Spline;
		const auto Vehicle = Context.VehiclePawn;

		Category.Add(TEXT("DistanceAlongSpline"), FString::Printf(TEXT("%.1f"), LastSplineState->DistanceAlongSpline));

		const auto Key = Spline->FindInputKeyClosestToWorldLocation(Vehicle->GetFrontWorldLocation());
		const auto DistanceAlongSpline = Spline->GetDistanceAlongSplineAtSplineInputKey(Key);

		Category.Add(TEXT("Completion %"), FString::Printf(TEXT("%.1f"), DistanceAlongSpline / Spline->GetSplineLength() * 100.0f));
	}

	Snapshot->Status.Add(Category);
}

#endif
