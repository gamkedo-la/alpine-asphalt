// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AA_RacerSplineFollowingComponent.h"

#include "AI/AA_AIRacerContext.h"
#include "AI/AA_RacerContextProvider.h"
#include "Components/SplineComponent.h"
#include "Actors/AA_TrackInfoActor.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/AlpineAsphaltLogger.h"
#include "Logging/LoggingUtils.h"
#include "Landscape.h"

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
			TEXT("%s-s: BeginPlay - Owner does not implement IAA_RacerContextProvider"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
		return;
	}

	Landscape = GetLandscapeActor();

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log, TEXT("%s-%s: BeginPlay: Landscape=%s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(Landscape));

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
			TEXT("%s-s: SetInitialMovementTarget - RaceTrack not set!"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
		return;
	}
	if (!Context.RaceTrack->Spline)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Error,
			TEXT("%s-s: SetInitialMovementTarget - RaceTrack=%s does not have a Spline set!"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *Context.RaceTrack->GetName());
		return;
	}

	LastSplineState = GetNextSplineState(Context);

	if (!LastSplineState)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Display,
			TEXT("%s-s: SetInitialMovementTarget - RaceTrack=%s Completed!"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *Context.RaceTrack->GetName());
		return;
	}

	UpdateMovementFromLastSplineState(Context);
}

void UAA_RacerSplineFollowingComponent::OnVehicleAvoidancePositionUpdated(AAA_WheeledVehiclePawn* VehiclePawn, const FAA_AIRacerAvoidanceContext& AvoidanceContext)
{
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

FVector UAA_RacerSplineFollowingComponent::ClampTargetToGround(const FVector& Position) const
{
	if (!Landscape || !RacerContextProvider)
	{
		return Position;
	}

	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActor(RacerContextProvider->GetRacerContext().VehiclePawn);

	constexpr float TraceOffset = 2000;
	const FVector TraceStart = Position + FVector(0, 0, TraceOffset);
	const FVector TraceEnd = Position - FVector(0, 0, TraceOffset);

	FHitResult HitResult;
	// TODO: This isn't working - maybe due to this https://forums.unrealengine.com/t/actorlinetracesingle-returning-inaccurate-hit-result/343365
	// Not super important as this is temp code until start following the race spline
	if (Landscape->ActorLineTraceSingle(HitResult, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility, CollisionQueryParams))
	{
		return HitResult.Location;
	}

	if (auto World = GetWorld(); World && World->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECollisionChannel::ECC_Visibility,
		CollisionQueryParams))
	{
		return HitResult.Location;
	}

	return Position;
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
	State.WorldLocation = Spline->GetWorldLocationAtDistanceAlongSpline(State.DistanceAlongSpline);

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

	const auto SplineLength = Spline->GetSplineLength();

	// check if at the end of the spline
	if (FMath::IsNearlyEqual(SplineLength, LastSplineState->DistanceAlongSpline))
	{
		return std::nullopt;
	}

	const auto NextIdealDistanceAlongSpline = NextDistanceAlongSplineOverride ? *NextDistanceAlongSplineOverride : LastSplineState->DistanceAlongSpline + LookaheadDistance;
	const auto NextDistanceAlongSpline = FMath::Min(NextIdealDistanceAlongSpline, SplineLength);

	FSplineState State;

	const auto Key = State.SplineKey = Spline->GetInputKeyValueAtDistanceAlongSpline(NextDistanceAlongSpline);
	State.DistanceAlongSpline = NextDistanceAlongSpline;
	State.SplineDirection = Spline->GetDirectionAtSplineInputKey(Key, ESplineCoordinateSpace::World);
	State.WorldLocation = Spline->GetWorldLocationAtDistanceAlongSpline(NextDistanceAlongSpline);

	return State;
}

void UAA_RacerSplineFollowingComponent::UpdateMovementFromLastSplineState(FAA_AIRacerContext& RacerContext)
{
	check(LastSplineState);

	// Adjust speed based on upcoming curvature
	const auto NewSpeed = FMath::Clamp(MaxSpeedMph * (1 - CalculateUpcomingRoadCurvature()), MinSpeedMph, MaxSpeedMph);

	RacerContext.DesiredSpeedMph = NewSpeed;
	RacerContext.MovementTarget = LastSplineState->WorldLocation;

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
	
	Category.Add(TEXT("LookaheadDistance"), FString::Printf(TEXT("%.1f"), LookaheadDistance));

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
