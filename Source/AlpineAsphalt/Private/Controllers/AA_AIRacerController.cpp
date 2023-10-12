// Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/AA_AIRacerController.h"

#include "AI/AA_AIVehicleControlComponent.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/AlpineAsphaltLogger.h"
#include "Logging/LoggingUtils.h"
#include "Landscape.h"

#include "Pawn/AA_WheeledVehiclePawn.h"

using namespace AA;

AAA_AIRacerController::AAA_AIRacerController()
{
	VehicleControlComponent = CreateDefaultSubobject<UAA_AIVehicleControlComponent>(TEXT("Vehicle Control"));
}

#if ENABLE_VISUAL_LOG
void AAA_AIRacerController::GrabDebugSnapshot(FVisualLogEntry* Snapshot) const
{
	Super::GrabDebugSnapshot(Snapshot);

	if (VehicleControlComponent)
	{
		VehicleControlComponent->DescribeSelfToVisLog(Snapshot);
	}
}
void AAA_AIRacerController::BeginPlay()
{
	UE_VLOG_UELOG(this, LogAlpineAsphalt, Log, TEXT("%s: BeginPlay"), *GetName());

	Super::BeginPlay();

	Landscape = GetLandscapeActor();

	UE_VLOG_UELOG(this, LogAlpineAsphalt, Log, TEXT("%s: BeginPlay: Landscape=%s"), *GetName(), *LoggingUtils::GetName(Landscape));
}

void AAA_AIRacerController::OnPossess(APawn* InPawn)
{
	UE_VLOG_UELOG(this, LogAlpineAsphalt, Log, TEXT("%s: OnPossess: %s"), *GetName(), *LoggingUtils::GetName(InPawn));

	Super::OnPossess(InPawn);

	auto VehiclePawn = Cast<AAA_WheeledVehiclePawn>(InPawn);

	if (!VehiclePawn)
	{
		UE_VLOG_UELOG(this, LogAlpineAsphalt, Error, TEXT("%s: OnPossess: %s (%s) is not a AAA_WheeledVehiclePawn!"),
			*GetName(), *LoggingUtils::GetName(InPawn), InPawn ? *LoggingUtils::GetName(InPawn->GetClass()) : TEXT("NULL"));
		return;
	}

	check(VehicleControlComponent);
	VehicleControlComponent->SetVehiclePawn(VehiclePawn);

	// TODO: Plan to use AI State Tree to manage AI behavior.  Since this is new to UE 5.1 and haven't used it, 
	// fallback would be to use a behavior tree or even just code up the logic in the AI Controller itself
	// Here we just set something to test

	VehicleControlComponent->OnVehicleReachedTarget.AddDynamic(this, &ThisClass::SelectNewMovementTarget);
	SelectNewMovementTarget(VehiclePawn, FVector::ZeroVector);
}

void AAA_AIRacerController::OnUnPossess()
{
	UE_VLOG_UELOG(this, LogAlpineAsphalt, Log, TEXT("%s: OnUnPossess: PreviousPawn=%s"), *GetName(), *LoggingUtils::GetName(GetPawn()));

	Super::OnUnPossess();

	if (VehicleControlComponent)
	{
		VehicleControlComponent->SetVehiclePawn(nullptr);
		VehicleControlComponent->OnVehicleReachedTarget.RemoveDynamic(this, &ThisClass::SelectNewMovementTarget);
	}
}

void AAA_AIRacerController::SelectNewMovementTarget(AAA_WheeledVehiclePawn* VehiclePawn, const FVector& PreviousMovementTarget)
{
	UE_VLOG_UELOG(this, LogAlpineAsphalt, Log, TEXT("%s: SelectNewMovementTarget: VehiclePawn=%s; PreviousMovementTarget=%s"),
		*GetName(), *LoggingUtils::GetName(VehiclePawn), *PreviousMovementTarget.ToCompactString());

	check(VehiclePawn);

	const FVector RawRandomTarget = VehiclePawn->GetFrontWorldLocation() +
		LookaheadDistance * VehiclePawn->GetActorForwardVector().RotateAngleAxis(FMath::FRandRange(-20.f, 20.f), FVector::ZAxisVector).GetSafeNormal();

	const FVector MovementTarget = ClampTargetToGround(RawRandomTarget);

	// Snap to Ground
	VehicleControlComponent->SetMovementTarget(MovementTarget);

	// make small speed adjustment
	const auto NewSpeed = FMath::Clamp(FMath::FRandRange(VehicleControlComponent->GetDesiredSpeedMph() * 0.8, VehicleControlComponent->GetDesiredSpeedMph() * 1.2), 10, 100);

	VehicleControlComponent->SetDesiredSpeedMph(FMath::FRandRange(VehicleControlComponent->GetDesiredSpeedMph() * 0.8, VehicleControlComponent->GetDesiredSpeedMph() * 1.2));
}

ALandscape* AAA_AIRacerController::GetLandscapeActor() const
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

FVector AAA_AIRacerController::ClampTargetToGround(const FVector& Position) const
{
	if (!Landscape)
	{
		return Position;
	}

	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActor(GetPawn());

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

#endif
