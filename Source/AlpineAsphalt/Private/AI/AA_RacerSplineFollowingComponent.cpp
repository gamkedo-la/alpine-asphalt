// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AA_RacerSplineFollowingComponent.h"

#include "AI/AA_AIRacerContext.h"
#include "AI/AA_RacerContextProvider.h"
#include "Components/SplineComponent.h"
#include "Actors/AA_TrackInfoActor.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/AlpineAsphaltLogger.h"
#include "Logging/LoggingUtils.h"
#include "FunctionLibraries/AA_BlueprintFunctionLibrary.h"
#include "Landscape.h"
#include "Components/SplineMeshComponent.h"

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

	Landscape = GetLandscapeActor();

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log, TEXT("%s-%s: BeginPlay: Landscape=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(Landscape));

	LastCurvature = 0.0f;
	MaxApproachAngleCosine = FMath::Cos(FMath::DegreesToRadians(MaxApproachAngle));

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
			TEXT("%s-%s: SelectNewMovementTarget - RaceTrack not set!"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
		return;
	}
	if (!Context.RaceTrack->Spline)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Error,
			TEXT("%s-%s: SelectNewMovementTarget - RaceTrack=%s does not have a Spline set!"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *Context.RaceTrack->GetName());
		return;
	}

	check(LastSplineState);

	const auto OriginalSplineState = LastSplineState;
	LastSplineState = GetNextSplineState(Context);

	if (!LastSplineState)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Display,
			TEXT("%s-%s: SelectNewMovementTarget - RaceTrack=%s Completed!"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *Context.RaceTrack->GetName());
		return;
	}
	
	UpdateLastSplineStateIfApproachTooSteep(Context, *VehiclePawn, *OriginalSplineState);
	UpdateMovementFromLastSplineState(Context);
}

void UAA_RacerSplineFollowingComponent::UpdateLastSplineStateIfApproachTooSteep(
	const FAA_AIRacerContext& RacerContext, const AAA_WheeledVehiclePawn& VehiclePawn, const FSplineState& OriginalSplineState)
{
	const auto ToMovementDirection = (LastSplineState->WorldLocation - VehiclePawn.GetFrontWorldLocation()).GetSafeNormal();

	if ((ToMovementDirection | VehiclePawn.GetActorForwardVector()) < MaxApproachAngleCosine)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
			TEXT("%s-%s: SelectNewMovementTarget - Approach angle too steep: %.1f > %.1f - select target further ahead"),
			*GetName(), *LoggingUtils::GetName(GetOwner()),
			FMath::RadiansToDegrees(FMath::Acos(ToMovementDirection | VehiclePawn.GetActorForwardVector())),
			MaxApproachAngle
		);

		// calculate further ahead
		if (const auto NextSplineState = GetNextSplineState(RacerContext, LastSplineState->DistanceAlongSpline + MaxLookaheadDistance); NextSplineState)
		{
			LastSplineState = NextSplineState;
		}
	}

	const auto NextTargetSplineDistanceDelta = LastSplineState->DistanceAlongSpline - OriginalSplineState.DistanceAlongSpline;

	// Make sure that our desired target doesn't result in a collision with a static world object from "cutting the corner"
	if (NextTargetSplineDistanceDelta > MinLookaheadDistance && IsCurrentPathOccluded(VehiclePawn, *LastSplineState))
	{
		// Walk it back to a minimum lookahead distance
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
			TEXT("%s-%s: SelectNewMovementTarget - Suggested path is occluded - recalculating closer to vehicle"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));

		UE_VLOG_LOCATION(GetOwner(), LogAlpineAsphalt, Log, LastSplineState->WorldLocation + FVector(0, 0, 50.0f), 100.0f, FColor::Purple,
			TEXT("%s - Original Target Collision"), *VehiclePawn.GetName());

		LastSplineState = OriginalSplineState;
		LastSplineState = GetNextSplineState(RacerContext, std::nullopt, MinLookaheadDistance);

		// should be able to recalculate
		check(LastSplineState);
	}
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

	const auto MaxDelta = CalculateMaxOffsetAtLastSplineState();

	if (MaxDelta <= 0)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Warning, TEXT("%s-%s: OnVehicleAvoidancePositionUpdated: Unable to offset as MaxDelta <= 0 - VehiclePawn=%s; AvoidanceContext=%s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()),
			*LoggingUtils::GetName(VehiclePawn), *AvoidanceContext.ToString());

		ResetAvoidanceContext();
		return;
	}

	auto& RacerContext = RacerContextProvider->GetRacerContext();
	const auto& RacerReferencePosition = VehiclePawn->GetFrontWorldLocation();
	const auto MovementDirection = (RacerContext.MovementTarget - RacerReferencePosition).GetSafeNormal();
	const auto ThreatAlignmentFactor = AvoidanceContext.ThreatVector | MovementDirection;

	// parallel - either pick opposite side of current road offset or deterministically pick left or right side of road
	if (FMath::IsNearlyEqual(ThreatAlignmentFactor, 1.0f))
	{
		if (FMath::IsNearlyZero(LastSplineState->RoadOffset))
		{
			CurrentAvoidanceOffset = MaxDelta * (reinterpret_cast<std::size_t>(VehiclePawn) % 2 == 0 ? 1 : -1) * AvoidanceContext.NormalizedThreatScore;
		}
		else
		{
			CurrentAvoidanceOffset = -MaxDelta * FMath::Sign(LastSplineState->RoadOffset) * AvoidanceContext.NormalizedThreatScore;
		}

		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose, TEXT("%s-%s: OnVehicleAvoidancePositionUpdated: Parallel to movement vector - set CurrentAvoidanceOffset=%f"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), CurrentAvoidanceOffset);
	}
	else
	{
		const auto AvoidanceTargetVector = FMath::GetReflectionVector(-AvoidanceContext.ThreatVector, MovementDirection);
		// cross product of reflection vector to choose which side of road to go on
		// Unreal uses Left hand rule since it is a left handed coordinate system so need to invert the order of cross product
		const auto CrossProduct = MovementDirection ^ AvoidanceTargetVector;
		CurrentAvoidanceOffset = MaxDelta * FMath::Sign(CrossProduct.Z) * AvoidanceContext.NormalizedThreatScore;

		UE_VLOG_ARROW(GetOwner(), LogAlpineAsphalt, Log,
			RacerReferencePosition + FVector(0, 0, 50.0f), 
			RacerReferencePosition + FVector(0, 0, 50.0f) + LastSplineState->LookaheadDistance * AvoidanceTargetVector.GetSafeNormal(), 
			FColor::Red, TEXT("%s - AvoidanceTargetVector"), *VehiclePawn->GetName());
	}

	LastAvoidanceContext = AvoidanceContext;

	UpdateSplineStateWithRoadOffset(RacerContext, *LastSplineState, CurrentAvoidanceOffset);
	UpdateLastSplineStateIfApproachTooSteep(RacerContext, *VehiclePawn, FSplineState { *LastSplineState } );

	UE_VLOG_ARROW(GetOwner(), LogAlpineAsphalt, Log, RacerReferencePosition + FVector(0, 0, 50.0f), RacerReferencePosition + FVector(0, 0, 50.0f) + AvoidanceContext.ThreatVector * LastSplineState->LookaheadDistance, FColor::Orange,
		TEXT("%s - ThreatVector - Score=%s"), *VehiclePawn->GetName(), *FString::Printf(TEXT("%.3f"), AvoidanceContext.NormalizedThreatScore));

	UE_VLOG_ARROW(GetOwner(), LogAlpineAsphalt, Log, RacerReferencePosition + FVector(0, 0, 50.0f), RacerContext.MovementTarget + FVector(0,0,50.0f), FColor::Green,
		TEXT("%s - MovementVector"), *VehiclePawn->GetName());

	UE_VLOG_LOCATION(GetOwner(), LogAlpineAsphalt, Log, LastSplineState->WorldLocation + FVector(0,0,50.0f), 100.0f, FColor::Red,
		TEXT("%s - Avoidance Target; CurrentAvoidanceOffset=%f"), *VehiclePawn->GetName(), CurrentAvoidanceOffset);


	UpdateMovementFromLastSplineState(RacerContext);
}

void UAA_RacerSplineFollowingComponent::ResetAvoidanceContext()
{
	CurrentAvoidanceOffset = 0;
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

	if (!IsSplineStateASufficientTarget(*Context.VehiclePawn, *LastSplineState))
	{
		LastSplineState = GetNextSplineState(Context, LastSplineState->DistanceAlongSpline + LastSplineState->LookaheadDistance);
		if (!LastSplineState)
		{
			return;
		}
	}

	// Initialize to current position for curvature calculation
	LastMovementTarget = Context.VehiclePawn->GetFrontWorldLocation();

	UpdateMovementFromLastSplineState(Context);
}

bool UAA_RacerSplineFollowingComponent::IsSplineStateASufficientTarget(const AAA_WheeledVehiclePawn& VehiclePawn, const FSplineState& SplineState) const
{
	const auto ToTargetDirection = (LastSplineState->WorldLocation - VehiclePawn.GetFrontWorldLocation()).GetSafeNormal();

	// Make sure target is far enough in front that we aren't backing up right at the start 
	if ((ToTargetDirection | VehiclePawn.GetActorForwardVector()) < MinInitialTargetAlignment)
	{
		return false;
	}

	// This isn't perfect as the road could curve but should be an okay approximation and it's just the start which is usually mostly straight anyway
	const auto CurrentDistance = FVector::Distance(LastSplineState->WorldLocation, VehiclePawn.GetFrontWorldLocation());
	// check the distance to the first point, if it is >= NextDistanceAlongSpline then just return; otherwise look to the next lookahead increment
	return CurrentDistance >= LastSplineState->LookaheadDistance;
}

bool UAA_RacerSplineFollowingComponent::IsCurrentPathOccluded(const AAA_WheeledVehiclePawn& VehiclePawn, const FSplineState& SplineState) const
{
	auto World = GetWorld();
	if (!World)
	{
		return false;
	}

	FCollisionQueryParams CollisionQueryParams;
	// Landscape ignore doesn't work as it comes in as a streaming proxy at runtime
	//CollisionQueryParams.AddIgnoredActor(Landscape);
	// Get all the overlaps with AIPath channel (ECC_GameTraceChannel1)
	CollisionQueryParams.bIgnoreBlocks = true;
	
	// Position at front of vehicle in middle
	const auto TraceStartLocation = VehiclePawn.GetFrontWorldLocation() + 
		VehiclePawn.GetActorUpVector() * VehiclePawn.GetVehicleHeight() * 0.5f;
	const auto& TraceEndLocation = SplineState.WorldLocation;

	// TODO: Ignoring the landscape actor is not working as it is hitting one of the streaming proxies instead
	// Also lots of hacky exclusions by name
	// Need to use multihit and manually ignore the landscape/road collisions and other random things that shouldn't match (USplineMeshComponent)
	// Ideally need a dedicated channel for trees, rocks, etc that could be static collision targets along the track
	TArray<FHitResult> HitResults;
	// AIPath channel
	World->LineTraceMultiByChannel(HitResults, TraceStartLocation, TraceEndLocation, ECollisionChannel::ECC_GameTraceChannel1, CollisionQueryParams);
	
	const auto NonLandscapeCollisionPredicate = [](const auto& HitResult)
	{
		if (!HitResult.Component.IsValid()) return true;

		auto Actor = HitResult.Component->GetOwner();
		if (!Actor) return true;

		const auto& ActorName = Actor->GetName();

		return Cast<USplineMeshComponent>(HitResult.Component) == nullptr &&
			Cast<AAA_TrackInfoActor>(Actor) == nullptr &&
			!ActorName.Contains("Checkpoint") &&
			!ActorName.Contains("Landscape") &&
			!ActorName.Contains("Tire");
	};

	const bool bOccluded = HitResults.ContainsByPredicate(NonLandscapeCollisionPredicate);

#if ENABLE_VISUAL_LOG

	if (bOccluded && FVisualLogger::IsRecording())
	{
		FHitResult* HitResultMatch = HitResults.FindByPredicate(NonLandscapeCollisionPredicate);
		check(HitResultMatch);

		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
			TEXT("%s-%s: IsCurrentPathOccluded - TRUE - Actor=%s; Component=%s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()),
			HitResultMatch->Component.IsValid() ? *LoggingUtils::GetName(HitResultMatch->Component->GetOwner()) : TEXT("NULL"),
			*LoggingUtils::GetName(HitResultMatch->Component.Get())
		);
	}
#endif

	return bOccluded;
}

ALandscape* UAA_RacerSplineFollowingComponent::GetLandscapeActor() const
{
	const auto GameWorld = GetWorld();

	for (TObjectIterator<ALandscape> It; It; ++It)
	{
		if (GameWorld == It->GetWorld())
		{
			return *It;
		}
	}
	return nullptr;
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
	State.LookaheadDistance = MinLookaheadDistance;

	return State;
}

std::optional<UAA_RacerSplineFollowingComponent::FSplineState> UAA_RacerSplineFollowingComponent::GetNextSplineState(
	const FAA_AIRacerContext& RacerContext, std::optional<float> NextDistanceAlongSplineOverride, std::optional<float> LookaheadDistanceOverride) const
{
	check(RacerContext.RaceTrack);
	check(RacerContext.RaceTrack->Spline);
	check(RacerContext.VehiclePawn);
	check(LastSplineState);

	auto Spline = RacerContext.RaceTrack->Spline;
	auto Vehicle = RacerContext.VehiclePawn;

	FSplineState State;

	// Adjust lookahead based on current curvature and speed of car
	// We need to actually lookahead further as curvature increases so that we can adjust for the car turning
	// Alpha of 1 is max distance and 0 is min distance lookahead
	if (!LookaheadDistanceOverride)
	{
		const auto LookaheadSpeedAlpha = FMath::Max(0, Vehicle->GetVehicleSpeedMph() - MinSpeedMph) / (MaxSpeedMph - MinSpeedMph);
		const auto LookaheadCurvatureAlpha = FMath::Abs(LastCurvature);
		const auto LookaheadAlpha = LookaheadCurvatureAlpha * LookaheadCurvatureAlphaWeight + LookaheadSpeedAlpha * (1 - LookaheadCurvatureAlphaWeight);

		State.LookaheadDistance = FMath::Lerp(MinLookaheadDistance, MaxLookaheadDistance, LookaheadAlpha);
	}
	else
	{
		State.LookaheadDistance = *LookaheadDistanceOverride;
	}

	float NextIdealDistanceAlongSpline;

	if (NextDistanceAlongSplineOverride)
	{
		NextIdealDistanceAlongSpline = *NextDistanceAlongSplineOverride;
	}
	else
	{
		// add distance that we are behind the spline
		const auto& CurrentPosition = Vehicle->GetFrontWorldLocation();
		const auto& ForwardVector = Vehicle->GetActorForwardVector();
		const auto ToLastSplineState = LastSplineState->WorldLocation - CurrentPosition;

		float DistanceBehind;
		if ((ToLastSplineState | ForwardVector) <= 0)
		{
			DistanceBehind = ToLastSplineState.Size();
		}
		else
		{
			DistanceBehind = 0;
		}

		NextIdealDistanceAlongSpline = LastSplineState->DistanceAlongSpline + DistanceBehind + State.LookaheadDistance;
	}

	// If at end of spline, then wrap around
	const auto NextDistanceAlongSpline = UAA_BlueprintFunctionLibrary::WrapEx(NextIdealDistanceAlongSpline, 0.0f, Spline->GetSplineLength());

	const auto Key = State.SplineKey = Spline->GetInputKeyValueAtDistanceAlongSpline(NextDistanceAlongSpline);
	State.DistanceAlongSpline = NextDistanceAlongSpline;
	State.SplineDirection = Spline->GetDirectionAtSplineInputKey(Key, ESplineCoordinateSpace::World);
	State.WorldLocation = State.OriginalWorldLocation = Spline->GetWorldLocationAtDistanceAlongSpline(NextDistanceAlongSpline);

	// Offset road target position
	UpdateSplineStateWithRoadOffset(RacerContext, State, CurrentAvoidanceOffset);

	return State;
}

void UAA_RacerSplineFollowingComponent::UpdateSplineStateWithRoadOffset(const FAA_AIRacerContext& RacerContext, FSplineState& SplineState, float RoadOffset) const
{
	SplineState.RoadOffset = RoadOffset;

	if (FMath::IsNearlyZero(RoadOffset))
	{
		return;
	}

	check(RacerContext.RaceTrack);
	auto Spline = RacerContext.RaceTrack->Spline;
	check(Spline);

	const auto& RightVector = Spline->GetRightVectorAtSplineInputKey(SplineState.SplineKey, ESplineCoordinateSpace::World);
	SplineState.WorldLocation = SplineState.OriginalWorldLocation + RightVector * RoadOffset;

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose, TEXT("%s-%s: UpdateSplineStateWithRoadOffset: RoadOffset=%f"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), RoadOffset);
}

void UAA_RacerSplineFollowingComponent::UpdateMovementFromLastSplineState(FAA_AIRacerContext& RacerContext)
{
	check(LastSplineState);
	check(RacerContext.VehiclePawn);

	// Adjust speed based on upcoming curvature
	LastCurvature = CalculateUpcomingRoadCurvature();

	// Adjust positioning based on curvature if not avoiding an obstacle 
	// head to outside of track to minimize the angular velocity and give best chance to negotiate the turn
	const auto CurrentAvoidanceOffsetSign = FMath::Sign(CurrentAvoidanceOffset);
	const auto LastCurvatureSign = FMath::Sign(LastCurvature);

	// Either adding a new offset or increasing the current one in the same direction
	if (!FMath::IsNearlyZero(LastCurvature) && (FMath::IsNearlyZero(CurrentAvoidanceOffsetSign) || FMath::IsNearlyEqual(CurrentAvoidanceOffsetSign, LastCurvatureSign)))
	{
		const auto CurvatureAdjustedOffset = LastCurvatureSign * FMath::Max(CurrentAvoidanceOffset * CurrentAvoidanceOffsetSign, CalculateMaxOffsetAtLastSplineState() * LastCurvature * LastCurvatureSign);

		// Blend with previous
		const auto AdjustedOffset = LastSplineState->RoadOffset * CurvatureRoadOffsetBlendFactor + CurvatureAdjustedOffset * (1 - CurvatureRoadOffsetBlendFactor);
		UpdateSplineStateWithRoadOffset(RacerContext, *LastSplineState, AdjustedOffset);
	}

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
		TEXT("%s-%s: UpdateMovementFromLastSplineState - Curvature=%f"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), LastCurvature);

	RacerContext.DesiredSpeedMph = CalculateNewSpeed(RacerContext);
	RacerContext.MovementTarget = LastSplineState->WorldLocation;
	RacerContext.DistanceAlongSpline = LastSplineState->DistanceAlongSpline;

	LastMovementTarget = RacerContext.MovementTarget;

	OnVehicleTargetUpdated.Broadcast(RacerContext.VehiclePawn, RacerContext.MovementTarget, RacerContext.DesiredSpeedMph);
}

float UAA_RacerSplineFollowingComponent::CalculateNewSpeed(const FAA_AIRacerContext& RacerContext) const
{
	check(RacerContext.VehiclePawn);

	// if target is behind us reduce speed to minimum
	const auto ToMovementTarget = RacerContext.MovementTarget - RacerContext.VehiclePawn->GetFrontWorldLocation();
	const auto TargetDotProduct = RacerContext.VehiclePawn->GetActorForwardVector() | ToMovementTarget;

	if (TargetDotProduct <= 0)
	{
		return MinSpeedMph;
	}

	// Make curvature influence max speed with square of straightness factor [0,1]
	const auto StraightnessFactor = 1 - FMath::Abs(LastCurvature);
	auto NewSpeed = ClampSpeed(MaxSpeedMph * FMath::Square(StraightnessFactor));

	// Scale the speed to avoidance target as well
	if (LastAvoidanceContext && NewSpeed > LastAvoidanceContext->NormalizedThreatSpeedMph)
	{
		const auto InitialNewSpeed = NewSpeed;
		NewSpeed = ClampSpeed(
			(NewSpeed * (1 - LastAvoidanceContext->NormalizedThreatScore) + LastAvoidanceContext->NormalizedThreatScore * LastAvoidanceContext->NormalizedThreatSpeedMph) *
			// reduce speed 10% for each threat
			(1 - AvoidanceThreatSpeedReductionFactor * LastAvoidanceContext->ThreatCount));

		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
			TEXT("%s-%s: UpdateMovementFromLastSplineState - Adjusting speed target based on avoidance data: InitialNewSpeed=%fmph; AdjustedNewSpeed=%fmph; AvoidanceContext=%s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), InitialNewSpeed, NewSpeed, *LastAvoidanceContext->ToString());
	}

	return NewSpeed;
}

float UAA_RacerSplineFollowingComponent::CalculateUpcomingRoadCurvature() const
{
	if (!LastSplineState || !RacerContextProvider)
	{
		return 0.0f;
	}

	const auto& RacerContext = RacerContextProvider->GetRacerContext();
	const auto& MyVehicle = RacerContext.VehiclePawn;

	if (!MyVehicle)
	{
		return 0.0f;
	}

	const auto LookaheadState = GetNextSplineState(RacerContext, LastSplineState->DistanceAlongSpline + LastSplineState->LookaheadDistance * RoadCurvatureLookaheadFactor);

	// Base curvature on direction vector to current target and direction from that target to one after it
	if (!LookaheadState)
	{
		return 0.0f;
	}

	const auto ToCurrentTargetNormalized = (LastSplineState->WorldLocation - LastMovementTarget).GetSafeNormal();
	const auto CurrentTargetToNextNormalized = (LookaheadState->WorldLocation - LastSplineState->WorldLocation).GetSafeNormal();
	const auto DotProduct = ToCurrentTargetNormalized | CurrentTargetToNextNormalized;

	// consider anything <= 0 a curvature of one
	// Sign is based on sign of cross product - left is positive and right is negative - this helps with offset calculations
	const auto Sign = -FMath::Sign((ToCurrentTargetNormalized ^ CurrentTargetToNextNormalized).Z);
	return Sign * FMath::Min(1 - DotProduct, 1);
}

float UAA_RacerSplineFollowingComponent::CalculateMaxOffsetAtLastSplineState() const
{
	const auto& RacerContext = RacerContextProvider->GetRacerContext();
	const auto RaceTrack = RacerContext.RaceTrack;

	if (!RaceTrack)
	{
		return 0.0f;
	}

	const auto VehiclePawn = RacerContext.VehiclePawn;

	if (!VehiclePawn)
	{
		return 0.0f;
	}

	const auto VehicleHalfWidth = VehiclePawn->GetVehicleWidth() * 0.5f;
	const auto RoadHalfWidth = RaceTrack->GetWidthAtDistance(LastSplineState->DistanceAlongSpline) * 0.5f;
	const auto MaxDelta = RoadHalfWidth - VehicleHalfWidth;

	if (MaxDelta <= 0)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Warning, TEXT("%s-%s: CalculateMaxOffsetAtLastSplineState: Unable to offset as VehicleHalfWidth=%f > RoadHalfWidth=%f - VehiclePawn=%s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()),
			VehicleHalfWidth, RoadHalfWidth,
			*LoggingUtils::GetName(VehiclePawn));

		return 0.0f;
	}

	return MaxDelta;
}

#if ENABLE_VISUAL_LOG

void UAA_RacerSplineFollowingComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("Racer Spline Following Component");
	
	Category.Add(TEXT("MaxSpeed"), FString::Printf(TEXT("%.1f mph"), MaxSpeedMph));
	Category.Add(TEXT("LastCurvature"), FString::Printf(TEXT("%.1f"), LastCurvature));
	Category.Add(TEXT("CurrentAvoidanceOffset"), FString::Printf(TEXT("%.1f"), CurrentAvoidanceOffset));

	if (RacerContextProvider && LastSplineState && RacerContextProvider->GetRacerContext().VehiclePawn && 
		RacerContextProvider->GetRacerContext().RaceTrack && RacerContextProvider->GetRacerContext().RaceTrack->Spline)
	{
		const auto& Context = RacerContextProvider->GetRacerContext();
		const auto Spline = Context.RaceTrack->Spline;
		const auto Vehicle = Context.VehiclePawn;

		Category.Add(TEXT("RoadOffset"), FString::Printf(TEXT("%.1f"), LastSplineState->RoadOffset));
		Category.Add(TEXT("Lookahead Distance"), FString::Printf(TEXT("%.1f"), LastSplineState->LookaheadDistance));
		Category.Add(TEXT("DistanceAlongSpline"), FString::Printf(TEXT("%.1f"), LastSplineState->DistanceAlongSpline));

		const auto Key = Spline->FindInputKeyClosestToWorldLocation(Vehicle->GetFrontWorldLocation());
		const auto DistanceAlongSpline = Spline->GetDistanceAlongSplineAtSplineInputKey(Key);

		Category.Add(TEXT("Completion %"), FString::Printf(TEXT("%.1f"), DistanceAlongSpline / Spline->GetSplineLength() * 100.0f));
		Category.Add(TEXT("Movement Target"), *LastSplineState->WorldLocation.ToCompactString());
	}

	Snapshot->Status.Add(Category);
}

#endif
