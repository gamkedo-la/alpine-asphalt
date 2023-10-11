// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AA_AIVehicleControlComponent.h"

#include "Logging/AlpineAsphaltLogger.h"
#include "Logging/LoggingUtils.h"
#include "Util/UnitConversions.h"
#include "VisualLogger/VisualLogger.h"

#include "Pawn/AA_WheeledVehiclePawn.h"

#include "Kismet/KismetMathLibrary.h"

using namespace AA;

// Sets default values for this component's properties
UAA_AIVehicleControlComponent::UAA_AIVehicleControlComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickInterval = 0.1f;
}

// Called when the game starts
void UAA_AIVehicleControlComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log, TEXT("%s-%s: BeginPlay"), *LoggingUtils::GetName(GetOwner()), *GetName());
}


void UAA_AIVehicleControlComponent::SetVehiclePawn(AAA_WheeledVehiclePawn* Pawn)
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log, TEXT("%s-%s: SetVehiclePawn: %s"), *LoggingUtils::GetName(GetOwner()), *GetName(), *LoggingUtils::GetName(Pawn));

	VehiclePawn = Pawn;
}

void UAA_AIVehicleControlComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	if (!bTargetSet || !VehiclePawn)
	{
		return;
	}

	// TODO: We will get the current max steering angle here from the vehicle pawn -> Vehicle movement component

	CalculateThrottle();
	CalculateSteering();
}

void UAA_AIVehicleControlComponent::CalculateThrottle() const
{
	check(VehiclePawn);

	const float CurrentSpeedMph = VehiclePawn->GetVehicleSpeedMph();
	const float ThrottleValue = SmoothThrottle(CurrentSpeedMph);

	if (ThrottleValue >= 0)
	{
		VehiclePawn->SetThrottle(ThrottleValue);
		VehiclePawn->SetBrake(0);
	}
	else
	{
		VehiclePawn->SetThrottle(0);
		VehiclePawn->SetBrake(-ThrottleValue);
	}
}

float UAA_AIVehicleControlComponent::SmoothThrottle(float CurrentSpeedMph) const
{
	float Delta = DesiredSpeedMph - CurrentSpeedMph;
	float Sign = FMath::Sign(Delta);

	float RawFactor = Delta / DesiredSpeedMph;
	float Factor = FMath::Pow(Sign * RawFactor, DeltaSpeedExponent);

	if (RawFactor >= RawFactorSwitchoverThreshold)
	{
		float RawThrottleFraction = FMath::Min(1.0f, DesiredSpeedMph / FullThrottleSpeedThresholdMph);
		float ThrottleFraction = FMath::Pow(RawThrottleFraction, FullThrottleExponent);

		return FMath::Min(FMath::Max(ThrottleFraction, Factor), 1.0f);
	}

	return FMath::Min(Factor, 1.0f) * Sign;
}

void UAA_AIVehicleControlComponent::CalculateSteering() const
{
	// Based on https://dev.epicgames.com/community/learning/tutorials/ryL5/unreal-engine-follow-a-spline-and-report-distance-along-it-using-an-actor-component
	check(VehiclePawn);

	const FVector DestinationDelta = CurrentMovementTarget - VehiclePawn->GetActorLocation();

	const float TargetYawDelta = GetTargetSteeringYawAngle(DestinationDelta);
	const float RawSteeringValue = UKismetMathLibrary::MapRangeClamped(TargetYawDelta, -CurrentMaxSteeringAngle, CurrentMaxSteeringAngle, -1.0f, 1.0f);

	// Take overall multiplier between [0,1] and then exponentially increase dampening at low distance to avoid wobbles
	const float SteeringRawMultiplier = FMath::Clamp(DestinationDelta.Size() / DampeningDistance, 0.0f, 1.0f);
	const float SteeringSmoothingValue = FMath::Pow(SteeringRawMultiplier, DampeningExponent);

	const float SteeringValue = RawSteeringValue * SteeringSmoothingValue;

	VehiclePawn->SetSteering(SteeringValue);
}

float UAA_AIVehicleControlComponent::GetTargetSteeringYawAngle(const FVector& DestinationDelta) const
{
	check(VehiclePawn);

	const FRotator TargetRotator = DestinationDelta.Rotation();
	const FRotator& CurrentRotation = VehiclePawn->GetActorRotation();

	// KismetMathLibrary (Delta (Rotator)) -> (A - B).Normalize()
	const FRotator DeltaRotation = (TargetRotator - CurrentRotation).GetNormalized();

	return DeltaRotation.Yaw;
}

bool UAA_AIVehicleControlComponent::HasReachedTarget() const
{
	check(VehiclePawn);

	return FVector::Distance(VehiclePawn->GetActorLocation(), CurrentMovementTarget) < TargetReachedRadius;
}

void UAA_AIVehicleControlComponent::CheckIfReachedTarget()
{
	if (bTargetReached || !HasReachedTarget())
	{
		return;
	}

	bTargetReached = true;

	check(VehiclePawn);

	OnVehicleReachedTarget.Broadcast(VehiclePawn, CurrentMovementTarget);
}

void UAA_AIVehicleControlComponent::SetDesiredSpeedMph(float SpeedMph)
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log, TEXT("%s-%s: SetDesiredSpeedMph: %fmph"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), SpeedMph);

	DesiredSpeedMph = SpeedMph;
}

void UAA_AIVehicleControlComponent::SetMovementTarget(const FVector& MovementTarget)
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log, TEXT("%s-%s: SetMovementTarget: %s"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *MovementTarget.ToCompactString());

	bTargetSet = true;

	if (MovementTarget == CurrentMovementTarget)
	{
		return;
	}

	CurrentMovementTarget = MovementTarget;
	bTargetReached = false;
}
