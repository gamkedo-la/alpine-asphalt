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
#include "Curves/CurveFloat.h"

#include "Pawn/AA_WheeledVehiclePawn.h"
#include "Util/SplineUtils.h"

using namespace AA;
using namespace AA_RacerSplineFollowingComponent;

namespace
{
	float CalculateCurvatureSign(const FVector& FirstDirection, const FVector& SecondDirection);
	float GetSplineDistanceBetween(float FirstDistance, float SecondDistance, float SplineLength);
	std::pair<float, float> CalculateCurvatureAndSignBetween(const FVector& FirstLocation, const FVector& SecondLocation, const FVector& ThirdLocation);
}

// Sets default values for this component's properties
UAA_RacerSplineFollowingComponent::UAA_RacerSplineFollowingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.2f;
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

		SetComponentTickEnabled(false);
		return;
	}

	Landscape = GetLandscapeActor();

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log, TEXT("%s-%s: BeginPlay: Landscape=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(Landscape));

	LastCurvature = {};
	MaxApproachAngleCosine = FMath::Cos(FMath::DegreesToRadians(MaxApproachAngle));

	RegisterRewindable(ERestoreTiming::Resume);

	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &ThisClass::SetInitialMovementTarget);
}

void UAA_RacerSplineFollowingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!RacerContextProvider)
	{
		return;
	}

	auto& Context = RacerContextProvider->GetRacerContext();
	const auto RaceTrack = Context.RaceTrack;
	const auto Vehicle = Context.VehiclePawn;

	if (!RaceTrack || !Vehicle)
	{
		return;
	}

	const auto Spline = RaceTrack->Spline;
	if (!Spline)
	{
		return;
	}

	SplineUtils::TryUpdateRaceState(*Spline, Context.RaceState);
}

void UAA_RacerSplineFollowingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UnregisterRewindable();
}

AA_RacerSplineFollowingComponent::FSnapshotData UAA_RacerSplineFollowingComponent::CaptureSnapshot() const
{
	return AA_RacerSplineFollowingComponent::FSnapshotData
	{
		.LastSplineState = LastSplineState,
		.LastAvoidanceContext = LastAvoidanceContext,
		.LastMovementTarget = LastMovementTarget,
		.LastCurvature = LastCurvature,
		.CurrentAvoidanceOffset = CurrentAvoidanceOffset
	};
}

void UAA_RacerSplineFollowingComponent::RestoreFromSnapshot(const AA_RacerSplineFollowingComponent::FSnapshotData& InSnapshotData, float InRewindTime)
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
		TEXT("%s-%s: RestoreFromSnapshot: InRewindTime=%f"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), InRewindTime);

	LastSplineState = InSnapshotData.LastSplineState;
	LastAvoidanceContext = InSnapshotData.LastAvoidanceContext;
	LastMovementTarget = InSnapshotData.LastMovementTarget;
	LastCurvature = InSnapshotData.LastCurvature;
	CurrentAvoidanceOffset = InSnapshotData.CurrentAvoidanceOffset;
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
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
			TEXT("%s-%s: SelectNewMovementTarget - Could not find next movement target - Skipping"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
		LastSplineState = OriginalSplineState;

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

		UE_VLOG_LOCATION(GetOwner(), LogAlpineAsphalt, Log, LastSplineState->WorldLocation + FVector(0, 0, 100.0f), 100.0f, FColor::Magenta,
			TEXT("%s - Original Target (Too Steep)"), *VehiclePawn.GetName());

		// Only calculate further ahead if haven't done previously - limit to MaxLookaheadSteepnessAdditionalMultiplier * MaxLookaheadDistance from current distance along spline
		const auto CurrentLookaheadDelta = LastSplineState->DistanceAlongSpline - RacerContext.RaceState.DistanceAlongSpline;
		// account for a possible MinLookaheadDistance initially
		if (CurrentLookaheadDelta >= MaxLookaheadDistance * MaxLookaheadSteepnessAdditionalMultiplier + MinLookaheadDistance)
		{
			UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
				TEXT("%s-%s: SelectNewMovementTarget - Not selecting target further ahead as current target=%s with CurrentLookaheadDelta=%f is already more than %f * %f + %f = %f spline distance ahead of current spline position=%f"),
				*GetName(), *LoggingUtils::GetName(GetOwner()),
				*LastSplineState->ToString(),
				CurrentLookaheadDelta,
				MaxLookaheadSteepnessAdditionalMultiplier,
				MaxLookaheadDistance,
				MinLookaheadDistance,
				CurrentLookaheadDelta,
				RacerContext.RaceState.DistanceAlongSpline
			);

			return;
		}

		// calculate further ahead
		if (const auto NextSplineState = GetNextSplineState(RacerContext, LastSplineState->DistanceAlongSpline + MaxLookaheadDistance * MaxLookaheadSteepnessAdditionalMultiplier); NextSplineState)
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
			TEXT("%s - Original Target (Collision)"), *VehiclePawn.GetName());

		LastSplineState = OriginalSplineState;
		LastSplineState = GetNextSplineState(RacerContext, std::nullopt, MinLookaheadDistance);

		// should be able to recalculate
		check(LastSplineState);
	}
}

bool UAA_RacerSplineFollowingComponent::ResetLastSplineStateToRaceState(FAA_AIRacerContext& RacerContext)
{
	check(RacerContext.RaceTrack);
	check(RacerContext.RaceTrack->Spline);
	check(RacerContext.VehiclePawn);

	auto Spline = RacerContext.RaceTrack->Spline;
	auto Vehicle = RacerContext.VehiclePawn;
	const auto& RaceState = RacerContext.RaceState;

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log, TEXT("%s-%s: ResetLastSplineStateToRaceState: VehiclePawn=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(Vehicle));

	const auto& ResetSplineWorldLocation = SplineUtils::ResetVehicleToLastSplineLocation(*Vehicle, *Spline, RaceState);

	FSplineState SplineState;

	SplineState.SplineKey = Spline->GetInputKeyAtDistanceAlongSpline(RaceState.MaxDistanceAlongSpline);
	SplineState.DistanceAlongSpline = RaceState.MaxDistanceAlongSpline;
	SplineState.WorldLocation =  SplineState.OriginalWorldLocation = ResetSplineWorldLocation;
	SplineState.LookaheadDistance = MinLookaheadDistance;
	SplineState.SplineDirection = Spline->GetDirectionAtSplineInputKey(SplineState.SplineKey, ESplineCoordinateSpace::World);

	ResetAvoidanceContext();

	LastSplineState = SplineState;

	const auto NextSplineState = GetNextSplineState(RacerContext, LastSplineState->DistanceAlongSpline + LastSplineState->LookaheadDistance);
	if (!NextSplineState)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Warning, TEXT("%s-%s: ResetLastSplineStateToRaceState: VehiclePawn=%s - Unable to GetNextSplineState from LastSplineState=%s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(Vehicle), *LastSplineState->ToString());

		return false;
	}

	LastSplineState = NextSplineState;

	return true;
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
			CurrentAvoidanceOffset = MaxDelta * (reinterpret_cast<std::size_t>(VehiclePawn) % 2 == 0 ? 1 : -1) * (AvoidanceContext.bNearThreat ? 1.0f : AvoidanceContext.NormalizedThreatScore);
		}
		else
		{
			CurrentAvoidanceOffset = -MaxDelta * FMath::Sign(LastSplineState->RoadOffset) * (AvoidanceContext.bNearThreat ? 1.0f : AvoidanceContext.NormalizedThreatScore);
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

void UAA_RacerSplineFollowingComponent::SelectUnstuckTarget(AAA_WheeledVehiclePawn* VehiclePawn, const FVector& IdealSeekPosition, bool bAtMaxRetries)
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log, TEXT("%s-%s: SelectUnstuckTarget: VehiclePawn=%s; IdealSeekPosition=%s, bAtMaxRetries=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(VehiclePawn), *IdealSeekPosition.ToCompactString(), LoggingUtils::GetBoolString(bAtMaxRetries));

	check(VehiclePawn);
	check(RacerContextProvider);
	auto& RacerContext = RacerContextProvider->GetRacerContext();

	check(RacerContext.RaceTrack);
	check(RacerContext.RaceTrack->Spline);

	auto RaceSpline = RacerContext.RaceTrack->Spline;

	if (bAtMaxRetries)
	{
		if (!ResetLastSplineStateToRaceState(RacerContext))
		{
			return;
		}
	}
	else
	{
		const auto Key = RaceSpline->FindInputKeyClosestToWorldLocation(IdealSeekPosition);
		const auto DistanceAlongSpline = RaceSpline->GetDistanceAlongSplineAtSplineInputKey(Key);

		auto NextSplineState = GetNextSplineState(RacerContext, DistanceAlongSpline);

		if (!NextSplineState)
		{
			return;
		}

		LastSplineState = NextSplineState;
	}

	UpdateMovementFromLastSplineState(RacerContext);
}

void UAA_RacerSplineFollowingComponent::OnTargetUnreachable(AAA_WheeledVehiclePawn* VehiclePawn, const FVector& CurrentMovementTarget)
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log, TEXT("%s-%s: OnTargetUnreachable: VehiclePawn=%s; CurrentMovementTarget=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(VehiclePawn), *CurrentMovementTarget.ToCompactString());

	check(VehiclePawn);
	check(RacerContextProvider);

	auto& RacerContext = RacerContextProvider->GetRacerContext();
	
	if (ResetLastSplineStateToRaceState(RacerContext))
	{
		UpdateMovementFromLastSplineState(RacerContext);
	}
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

	// Initialize to current position for curvature calculation
	LastMovementTarget = Context.VehiclePawn->GetFrontWorldLocation();
	Context.RaceState.SplineLength = SplineUtils::GetSplineLength(*Context.RaceTrack);

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

std::optional<FSplineState> UAA_RacerSplineFollowingComponent::GetInitialSplineState(const FAA_AIRacerContext& RacerContext) const
{
	check(RacerContext.RaceTrack);
	check(RacerContext.RaceTrack->Spline);
	check(RacerContext.VehiclePawn);

	auto Spline = RacerContext.RaceTrack->Spline;
	auto Vehicle = RacerContext.VehiclePawn;

	FSplineState State;

	const auto& CurrentVehicleLocation = Vehicle->GetFrontWorldLocation();
	const auto CurrentSplineKey = Spline->FindInputKeyClosestToWorldLocation(CurrentVehicleLocation);
	const auto CurrentDistanceAlongSpline = Spline->GetDistanceAlongSplineAtSplineInputKey(CurrentSplineKey);

	State.DistanceAlongSpline = UAA_BlueprintFunctionLibrary::WrapEx(CurrentDistanceAlongSpline + MinLookaheadDistance, 0.0f, Spline->GetSplineLength());
	const auto Key = State.SplineKey = Spline->GetInputKeyAtDistanceAlongSpline(State.DistanceAlongSpline);

	State.SplineDirection = Spline->GetDirectionAtSplineInputKey(Key, ESplineCoordinateSpace::World);
	State.WorldLocation = State.OriginalWorldLocation = Spline->GetWorldLocationAtDistanceAlongSpline(State.DistanceAlongSpline);
	State.LookaheadDistance = MinLookaheadDistance;

	return State;
}

std::optional<FSplineState> UAA_RacerSplineFollowingComponent::GetNextSplineState(
	const FAA_AIRacerContext& RacerContext, std::optional<float> NextDistanceAlongSplineOverride, std::optional<float> LookaheadDistanceOverride, bool bIgnoreRaceEnd) const
{
	check(RacerContext.RaceTrack);
	check(RacerContext.RaceTrack->Spline);
	check(RacerContext.VehiclePawn);
	check(LastSplineState);

	const auto RaceTrack = RacerContext.RaceTrack;
	auto Spline = RaceTrack->Spline;
	auto Vehicle = RacerContext.VehiclePawn;

	FSplineState State;

	// Adjust lookahead based on current curvature and speed of car
	// We need to actually lookahead further as curvature increases so that we can adjust for the car turning
	// Alpha of 1 is max distance and 0 is min distance lookahead
	if (!LookaheadDistanceOverride)
	{
		const auto LookaheadSpeedAlpha = FMath::Max(0, Vehicle->GetVehicleSpeedMph() - MinSpeedMph) / (MaxSpeedMph - MinSpeedMph);
		const auto LookaheadCurvatureAlpha = FMath::Abs(LastCurvature.Curvature);
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

	float NextDistanceAlongSpline;

	const auto SplineLength = RacerContext.RaceState.SplineLength;

	// If at end of lap, then wrap around if a circuit
	// Don't detect this if behind the starting line (GetTotalDistance() > 0)
	if (NextIdealDistanceAlongSpline >= SplineLength 
		&& ((!bIgnoreRaceEnd && RacerContext.RaceState.IsOnLastLap()) || (bIgnoreRaceEnd && !RacerContext.RaceState.IsLooping()))
		&& RacerContext.RaceState.GetTotalDistance() > 0)
	{
		if (!FMath::IsNearlyEqual(LastSplineState->DistanceAlongSpline, SplineLength))
		{
			NextDistanceAlongSpline = SplineLength;
		}
		// If already at end then return nullopt as race is over
		else
		{
			return std::nullopt;
		}
	}
	else
	{
		NextDistanceAlongSpline = UAA_BlueprintFunctionLibrary::WrapEx(NextIdealDistanceAlongSpline, 0.0f, SplineLength);
	}

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

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose, TEXT("%s-%s: UpdateSplineStateWithRoadOffset: RoadOffset=%f; SplineState=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), RoadOffset, *SplineState.ToString());
}

void UAA_RacerSplineFollowingComponent::UpdateMovementFromLastSplineState(FAA_AIRacerContext& RacerContext)
{
	check(LastSplineState);
	check(RacerContext.VehiclePawn);

	// Adjust speed based on upcoming curvature
	LastCurvature = CalculateUpcomingRoadCurvatureAndBankAngle();

	// Adjust positioning based on curvature if not avoiding an obstacle 
	// head to outside of track to minimize the angular velocity and give best chance to negotiate the turn
	const auto CurrentAvoidanceOffsetSign = FMath::Sign(CurrentAvoidanceOffset);
	const auto LastCurvatureSign = FMath::Sign(LastCurvature.Curvature);

	// Either adding a new offset or increasing the current one in the same direction
	if (!FMath::IsNearlyZero(LastCurvature.Curvature) && (FMath::IsNearlyZero(CurrentAvoidanceOffsetSign) || FMath::IsNearlyEqual(CurrentAvoidanceOffsetSign, LastCurvatureSign)))
	{
		const auto CurvatureAdjustedOffset = LastCurvatureSign * FMath::Max(CurrentAvoidanceOffset * CurrentAvoidanceOffsetSign, CalculateMaxOffsetAtLastSplineState() * LastCurvature.Curvature * LastCurvatureSign);

		// Blend with previous
		const auto AdjustedOffset = LastSplineState->RoadOffset * CurvatureRoadOffsetBlendFactor + CurvatureAdjustedOffset * (1 - CurvatureRoadOffsetBlendFactor);
		UpdateSplineStateWithRoadOffset(RacerContext, *LastSplineState, AdjustedOffset);
	}

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
		TEXT("%s-%s: UpdateMovementFromLastSplineState - Curvature=%f"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), LastCurvature.Curvature);

	RacerContext.DesiredSpeedMph = CalculateNewSpeed(RacerContext);
	RacerContext.MovementTarget = LastSplineState->WorldLocation;
	RacerContext.TargetDistanceAlongSpline = LastSplineState->DistanceAlongSpline;

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


	float StraightnessFactor = 1 - FMath::Abs(LastCurvature.Curvature);
	float NewSpeed;

	if (FMath::Abs(1.0f - StraightnessFactor) < 0.02f)
	{
		NewSpeed = MaxSpeedMph;
	}
	else if (SpeedVsCurvatureCurve)
	{
		if (CurvatureReductionVsBankAngleCurve && LastCurvature.BankAngle > 0)
		{
			StraightnessFactor = FMath::Min(StraightnessFactor * CurvatureReductionVsBankAngleCurve->FloatCurve.Eval(LastCurvature.BankAngle, 1.0f), 1.0f);
		}

		NewSpeed = ClampSpeed(SpeedVsCurvatureCurve->FloatCurve.Eval(1 - StraightnessFactor, MaxSpeedMph));
	}
	else
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Warning,
			TEXT("%s-%s: CalculateNewSpeed - SpeedVsCurvatureCurve not set - using default calculation"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));

		NewSpeed = GetDefaultSpeedFromStraightnessFactor(StraightnessFactor);
	}

	// Scale the speed to avoidance target as well
	if (LastAvoidanceContext && NewSpeed > LastAvoidanceContext->NormalizedThreatSpeedMph)
	{
		const auto InitialNewSpeed = NewSpeed;
		NewSpeed = ClampSpeed(
			(NewSpeed * (1 - LastAvoidanceContext->NormalizedThreatScore) + LastAvoidanceContext->NormalizedThreatScore * LastAvoidanceContext->NormalizedThreatSpeedMph) *
			// reduce speed 10% for each threat
			(1 - AvoidanceThreatSpeedReductionFactor * LastAvoidanceContext->ThreatCount));

		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
			TEXT("%s-%s: CalculateNewSpeed - Adjusting speed target based on avoidance data: InitialNewSpeed=%fmph; AdjustedNewSpeed=%fmph; AvoidanceContext=%s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), InitialNewSpeed, NewSpeed, *LastAvoidanceContext->ToString());
	}

	return NewSpeed;
}

float UAA_RacerSplineFollowingComponent::GetDefaultSpeedFromStraightnessFactor(float StraightnessFactor) const
{
	// Make curvature influence max speed with square of straightness factor [0,1]
	return ClampSpeed(MaxSpeedMph * FMath::Square(StraightnessFactor));
}

FRoadCurvature UAA_RacerSplineFollowingComponent::CalculateUpcomingRoadCurvatureAndBankAngle() const
{
	if (!LastSplineState || !RacerContextProvider)
	{
		return {};
	}

	const auto& RacerContext = RacerContextProvider->GetRacerContext();
	const auto& MyVehicle = RacerContext.VehiclePawn;

	if (!MyVehicle)
	{
		return {};
	}

	check(RacerContext.RaceTrack);
	check(RacerContext.RaceTrack->Spline);

	const auto RaceTrack = RacerContext.RaceTrack;
	const auto& RaceState = RacerContext.RaceState;

	float LookaheadDistanceAlongSpline = LastSplineState->DistanceAlongSpline + LastSplineState->LookaheadDistance * RoadCurvatureLookaheadFactor;
	const auto SplineLength = RacerContext.RaceState.SplineLength;

	if (LookaheadDistanceAlongSpline >= SplineLength && !RaceState.IsLooping())
	{
		// need to clamp to the end or GetNextSplineState can return nullopt
		LookaheadDistanceAlongSpline = SplineLength - 1;
	}

	const auto LookaheadState = GetNextSplineState(RacerContext, LookaheadDistanceAlongSpline, std::nullopt, true);

	// Base curvature on direction vector to current target and direction from that target to one after it
	if (!LookaheadState)
	{
		return {};
	}

	const auto [CurvatureValue, CurvatureSign] = CalculateCurvatureValueAndSign(RacerContext, *LookaheadState, SplineLength);

	const auto Spline = RaceTrack->Spline;

	// Determine if bank angle is positive or negative by the z component of the right vector and then multiple by the direction of the curve (CurvatureSign)
	// A left turn should be banked with right vector z positive and right turn with right vector z negative
	const auto LookaheadSplineRightVector = Spline->GetRightVectorAtSplineInputKey(LookaheadState->SplineKey, ESplineCoordinateSpace::Type::Local);

	FRoadCurvature Curvature;
	Curvature.Curvature = CurvatureValue;

	if (!FMath::IsNearlyZero(LookaheadSplineRightVector.Z))
	{
		const auto BankSign = FMath::Sign(LookaheadSplineRightVector.Z);

		// Bank angle is the angle between the world up and the spline world up
		const auto SplineUp = Spline->GetUpVectorAtSplineInputKey(LookaheadState->SplineKey, ESplineCoordinateSpace::Type::World);

		Curvature.BankAngle = FMath::RadiansToDegrees(FMath::Acos(SplineUp | FVector::UpVector) * CurvatureSign * BankSign);
	}
	else
	{
		// no bank
		Curvature.BankAngle = 0;
	}

	return Curvature;
}

std::pair<float, float> UAA_RacerSplineFollowingComponent::CalculateCurvatureValueAndSign(const FAA_AIRacerContext& Context, 
	const AA_RacerSplineFollowingComponent::FSplineState& LookaheadState, float SplineLength) const
{
	check(Context.RaceTrack);
	check(Context.RaceTrack->Spline);
	check(LastSplineState);

	// Calculate curvature by splitting up distance interval by N and then computing world location at distance along spline
	// Calculate dot product between adjacent direction vectors from last 3 points
	// If the curvature changes sign significantly between positive and negative, then consider it max curvature
	// If a local maximum curvature has a larger value than overall curvature, then use this value
	// Note that Spline::GetWorldLocationAtDistanceAlongSpline is performance intensive but there aren't many AI racers - profile to determine if it's an issue

	const auto Spline = Context.RaceTrack->Spline;

	const auto StartSplineDistance = LastSplineState->DistanceAlongSpline;
	const auto EndSplineDistance = LookaheadState.DistanceAlongSpline;

	const auto TotalSplineDistance = GetSplineDistanceBetween(StartSplineDistance, EndSplineDistance, SplineLength);
	const auto SamplingDeltaDistance = RoadCurvatureSubsamplingSplineDistance > 0 ? RoadCurvatureSubsamplingSplineDistance : TotalSplineDistance;

	std::pair<float, float> MaxLocalCurvatureAndSign{};

	FVector FirstPosition = LastMovementTarget;
	FVector SecondPosition = LastSplineState->WorldLocation;

	for (float SplineDeltaDistance = SamplingDeltaDistance; SplineDeltaDistance < TotalSplineDistance; SplineDeltaDistance += SamplingDeltaDistance)
	{
		const float ActualSplineDistance = UAA_BlueprintFunctionLibrary::WrapEx(StartSplineDistance + SplineDeltaDistance, 0.0f, SplineLength);
		const FVector NewLocation = Spline->GetWorldLocationAtDistanceAlongSpline(ActualSplineDistance);

		const auto NewCurvatureValueAndSign = CalculateCurvatureAndSignBetween(FirstPosition, SecondPosition, NewLocation);

		UE_VLOG_LOCATION(GetOwner(), LogAlpineAsphalt, VeryVerbose, NewLocation, 50.0f, FColor::Silver, TEXT("C=%f"), NewCurvatureValueAndSign.first);

		// First Iteration
		if (FMath::IsNearlyZero(MaxLocalCurvatureAndSign.second))
		{
			MaxLocalCurvatureAndSign = NewCurvatureValueAndSign;
		}
		// S-Curve check
		// check change of sign and significant change of magnitude (e.g. to avoid 0.01 -> -0.01 being triggered as a winding curve)
		else if (!FMath::IsNearlyEqual(NewCurvatureValueAndSign.second, MaxLocalCurvatureAndSign.second) &&
			FMath::Abs(NewCurvatureValueAndSign.first - MaxLocalCurvatureAndSign.first) > RoadCurvatureSignSwitchMagnitudeThreshold)
		{
			// 1 or -1 depending on sign
			MaxLocalCurvatureAndSign.first = MaxLocalCurvatureAndSign.second;
			return MaxLocalCurvatureAndSign;
		}
		// Take first sign and max overall magnitude
		else if (FMath::Abs(NewCurvatureValueAndSign.first) > FMath::Abs(MaxLocalCurvatureAndSign.first))
		{
			// multiply the signs so that only the magnitude is affected as the signs will cancel out
			MaxLocalCurvatureAndSign.first = MaxLocalCurvatureAndSign.second * NewCurvatureValueAndSign.second * NewCurvatureValueAndSign.first;
		}

		// rotate positions
		FirstPosition = SecondPosition;
		SecondPosition = NewLocation;
	}

	// calculate overall curvature and sign looking for biggest difference between local curvature and overall curvature
	const auto OverallCurvatureAndSign = CalculateCurvatureAndSignBetween(LastMovementTarget, LastSplineState->WorldLocation, LookaheadState.WorldLocation);

	UE_VLOG_LOCATION(GetOwner(), LogAlpineAsphalt, VeryVerbose, LookaheadState.WorldLocation, 50.0f, FColor::White, TEXT("C=%f"), OverallCurvatureAndSign.first);

	if (FMath::Abs(OverallCurvatureAndSign.first) > FMath::Abs(MaxLocalCurvatureAndSign.first))
	{
		return OverallCurvatureAndSign;
	}

	return MaxLocalCurvatureAndSign;
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

	const auto VehicleHalfWidth = VehiclePawn->GetVehicleWidth() * 0.25f;
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
	
	Category.Add(TEXT("Max Speed"), FString::Printf(TEXT("%.1f mph"), MaxSpeedMph));
	Category.Add(TEXT("Last Curvature"), FString::Printf(TEXT("%.1f"), LastCurvature.Curvature));
	Category.Add(TEXT("Last Bank Angle"), FString::Printf(TEXT("%.1f"), LastCurvature.BankAngle));
	Category.Add(TEXT("CurrentAvoidanceOffset"), FString::Printf(TEXT("%.1f"), CurrentAvoidanceOffset));

	if (RacerContextProvider && LastSplineState && RacerContextProvider->GetRacerContext().VehiclePawn && 
		RacerContextProvider->GetRacerContext().RaceTrack && RacerContextProvider->GetRacerContext().RaceTrack->Spline)
	{
		const auto& Context = RacerContextProvider->GetRacerContext();
		const auto Spline = Context.RaceTrack->Spline;
		const auto Vehicle = Context.VehiclePawn;

		Category.Add(TEXT("RoadOffset"), FString::Printf(TEXT("%.1f"), LastSplineState->RoadOffset));
		Category.Add(TEXT("Lookahead Distance"), FString::Printf(TEXT("%.1f"), LastSplineState->LookaheadDistance));
		Category.Add(TEXT("TargetDistanceAlongSpline"), FString::Printf(TEXT("%.1f"), LastSplineState->DistanceAlongSpline));

		Category.Add(TEXT("Movement Target"), *LastSplineState->WorldLocation.ToCompactString());

		const auto& RaceState = Context.RaceState;

		Category.Add(TEXT("Lap"), FString::Printf(TEXT("%d"), RaceState.LapCount + 1));
		Category.Add(TEXT("Lap Completion %"), FString::Printf(TEXT("%.1f"), RaceState.GetCurrentLapCompletionFraction() * 100));
		Category.Add(TEXT("Lap DistanceAlongSpline"), FString::Printf(TEXT("%.1f"), RaceState.DistanceAlongSpline));
		Category.Add(TEXT("Total DistanceAlongSpline"), FString::Printf(TEXT("%.1f"), RaceState.GetTotalDistance()));
	}

	Snapshot->Status.Add(Category);
}

#endif

FString FSplineState::ToString() const
{
	return FString::Printf(
		TEXT("OriginalWorldLocation=%s; WorldLocation=%s; SplineDirection=%s; SplineKey=%f; DistanceAlongSpline=%f; RoadOffset=%f; LookaheadDistance=%f"),
		*OriginalWorldLocation.ToCompactString(), *WorldLocation.ToCompactString(), *SplineDirection.ToCompactString(),
		SplineKey, DistanceAlongSpline, RoadOffset, LookaheadDistance);
}

namespace
{
	inline float CalculateCurvatureSign(const FVector& FirstDirection, const FVector& SecondDirection)
	{
		// CurvatureSign is based on sign of cross product - left is positive and right is negative - this helps with offset calculations
		return -FMath::Sign((FirstDirection ^ SecondDirection).Z);
	}

	float GetSplineDistanceBetween(float FirstDistance, float SecondDistance, float SplineLength)
	{
		if (SecondDistance >= FirstDistance)
		{
			return SecondDistance - FirstDistance;
		}

		// wrapping around
		return SplineLength - FirstDistance + SecondDistance;
	}

	std::pair<float, float> CalculateCurvatureAndSignBetween(const FVector& FirstLocation, const FVector& SecondLocation, const FVector& ThirdLocation)
	{
		const auto ToCurrentTargetNormalized = (SecondLocation - FirstLocation).GetSafeNormal();
		const auto CurrentTargetToNextNormalized = (ThirdLocation - SecondLocation).GetSafeNormal();
		const auto DotProduct = ToCurrentTargetNormalized | CurrentTargetToNextNormalized;

		const auto CurvatureSign = CalculateCurvatureSign(ToCurrentTargetNormalized, CurrentTargetToNextNormalized);
		// consider anything <= 0 a curvature of one
		const auto CurvatureValue = CurvatureSign * FMath::Min(1 - DotProduct, 1);

		return
		{
			CurvatureValue,
			CurvatureSign
		};
	}
}
