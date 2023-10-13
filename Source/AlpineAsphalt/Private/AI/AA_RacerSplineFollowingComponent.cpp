// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AA_RacerSplineFollowingComponent.h"

#include "AI/AA_AIRacerContext.h"
#include "AI/AA_RacerContextProvider.h"

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

	const FVector RawRandomTarget = VehiclePawn->GetFrontWorldLocation() +
		LookaheadDistance * VehiclePawn->GetActorForwardVector().RotateAngleAxis(FMath::FRandRange(-20.f, 20.f), FVector::ZAxisVector).GetSafeNormal();

	const FVector MovementTarget = ClampTargetToGround(RawRandomTarget);

	// make small speed adjustment
	const auto NewSpeed = FMath::Clamp(FMath::FRandRange(Context.DesiredSpeedMph * 0.8, Context.DesiredSpeedMph * 1.2), 10, 100);

	Context.DesiredSpeedMph = NewSpeed;
	Context.MovementTarget = MovementTarget;

	OnVehicleTargetUpdated.Broadcast(VehiclePawn, MovementTarget, NewSpeed);

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

	SelectNewMovementTarget(RacerContextProvider->GetRacerContext().VehiclePawn, FVector::ZeroVector);
}

#if ENABLE_VISUAL_LOG

void UAA_RacerSplineFollowingComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("Racer Spline Following Component");

	Snapshot->Status.Add(Category);
}

#endif
