// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AA_AIVehicleControlComponent.h"

#include "Logging/AlpineAsphaltLogger.h"
#include "Logging/LoggingUtils.h"
#include "Util/UnitConversions.h"
#include "VisualLogger/VisualLogger.h"


#include "ChaosWheeledVehicleMovementComponent.h"
#include "Pawn/AA_WheeledVehiclePawn.h"

#include "Kismet/KismetMathLibrary.h"

using namespace AA;

DEFINE_VLOG_EVENT(EventVehicleTargetReached, Display, "Target Reached")


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

void UAA_AIVehicleControlComponent::OnVehicleTargetUpdated(AAA_WheeledVehiclePawn* TheVehiclePawn, const FVector& MovementTarget, float NewDesiredSpeedMph)
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log, TEXT("%s-%s: OnVehicleTargetUpdated - MovementTarget=%s; NewDesiredSpeedMph=%f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(), *MovementTarget.ToCompactString(), NewDesiredSpeedMph);

	SetDesiredSpeedMph(NewDesiredSpeedMph);
	SetMovementTarget(MovementTarget);
}

void UAA_AIVehicleControlComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	if (!bTargetSet || !VehiclePawn)
	{
		return;
	}

	// TODO: We will get the current max steering angle here from the vehicle pawn -> Vehicle movement component
	CheckIfReachedTarget();

	UE_VLOG_LOCATION(GetOwner(), LogAlpineAsphalt, Log, CurrentMovementTarget, 100.0f, FColor::Green, TEXT("%s: MovementTarget"),
		*LoggingUtils::GetName(VehiclePawn));


	const FVector DestinationDelta = CurrentMovementTarget - VehiclePawn->GetFrontWorldLocation();

	if (ShouldReverseThrottleAndSteering(DestinationDelta))
	{
		bTurningAround = true;

		CalculateReverseThrottleAndSteering(DestinationDelta);
	}
	else
	{
		bTurningAround = false;
		CalculateThrottle();
		CalculateSteering(DestinationDelta);
	}
}

void UAA_AIVehicleControlComponent::CalculateThrottle() const
{
	check(VehiclePawn);

	const float CurrentSpeedMph = VehiclePawn->GetVehicleSpeedMph();
	const float ThrottleValue = SmoothThrottle(CurrentSpeedMph);

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, VeryVerbose,
		TEXT("%s-%s: CalculateThrottle: ThrottleValue=%f; CurrentSpeedMph=%f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		ThrottleValue, CurrentSpeedMph);

	SetSpeedControls(ThrottleValue);
}

float UAA_AIVehicleControlComponent::SmoothThrottle(float CurrentSpeedMph) const
{
	const auto Delta = DesiredSpeedMph - CurrentSpeedMph;
	const auto Sign = FMath::Sign(Delta);

	const auto RawFactor = Delta / DesiredSpeedMph;
	const auto Factor = FMath::Pow(Sign * RawFactor, DeltaSpeedExponent);

	if (RawFactor >= RawFactorSwitchoverThreshold)
	{
		const auto RawThrottleFraction = FMath::Min(1.0f, DesiredSpeedMph / FullThrottleSpeedThresholdMph);
		const auto ThrottleFraction = FMath::Pow(RawThrottleFraction, FullThrottleExponent);

		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, VeryVerbose,
			TEXT("%s-%s: SmoothThrottle(RawFactor): Delta=%f; RawFactor=%f; Factor=%f; RawThrottleFraction=%f; ThrottleFraction=%f"),
			*LoggingUtils::GetName(GetOwner()), *GetName(),
			Delta, RawFactor, Factor, RawThrottleFraction, ThrottleFraction);

		return FMath::Min(FMath::Max(ThrottleFraction, Factor), 1.0f);
	}

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, VeryVerbose,
		TEXT("%s-%s: SmoothThrottle(Factor): Delta=%f; RawFactor=%f; Factor=%f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		Delta, RawFactor, Factor);

	return FMath::Min(Factor, 1.0f) * Sign;
}

void UAA_AIVehicleControlComponent::CalculateSteering(const FVector& DestinationDelta) const
{
	// Based on https://dev.epicgames.com/community/learning/tutorials/ryL5/unreal-engine-follow-a-spline-and-report-distance-along-it-using-an-actor-component
	check(VehiclePawn);

	const float TargetYawDelta = GetTargetSteeringYawAngle(DestinationDelta);
	const float RawSteeringValue = UKismetMathLibrary::MapRangeClamped(TargetYawDelta, -CurrentMaxSteeringAngle, CurrentMaxSteeringAngle, -1.0f, 1.0f);

	// Take overall multiplier between [0,1] and then exponentially increase dampening at low distance to avoid wobbles
	const float SteeringRawMultiplier = FMath::Clamp(DestinationDelta.Size() / DampeningDistance, 0.0f, 1.0f);
	const float SteeringSmoothingValue = FMath::Pow(SteeringRawMultiplier, DampeningExponent);

	const float SteeringValue = RawSteeringValue * SteeringSmoothingValue;

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, VeryVerbose,
		TEXT("%s-%s: CalculateSteering: SteeringValue=%f; DestinationDelta=%s; TargetYawDelta=%f; RawSteeringValue=%f; SteeringRawMultiplier=%f; SteeringSmoothingValue=%f"),
		*LoggingUtils::GetName(GetOwner()), *GetName(),
		SteeringValue, *DestinationDelta.ToCompactString(), TargetYawDelta, RawSteeringValue, SteeringRawMultiplier, SteeringSmoothingValue);

	VehiclePawn->SetSteering(SteeringValue);
}

bool UAA_AIVehicleControlComponent::ShouldReverseThrottleAndSteering(const FVector& DestinationDelta) const
{
	check(VehiclePawn);

	const float TurnAroundThreshold = bTurningAround ? ContinueTurningAroundCosineThreshold : TurnAroundCosineThreshold;
	const float ForwardFactor = FVector::DotProduct(VehiclePawn->GetActorForwardVector(), DestinationDelta.GetSafeNormal());

	if (ForwardFactor >= TurnAroundThreshold)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, VeryVerbose,
			TEXT("%s-%s: ShouldReverseThrottleAndSteering - Result=false; bTurningAround=%s; ForwardFactor=%f; TurnAroundFactor=%f"),
			*GetName(),
			*VehiclePawn->GetName(),
			LoggingUtils::GetBoolString(bTurningAround),
			ForwardFactor,
			TurnAroundThreshold
		);

		return false;
	}

	// Turn around to reach destination if it is inside the turning circle radius
	const float TargetDistance = DestinationDelta.Size();
	// TODO: Calculate turning circle radius from current steering angle and vehicle properties
	const float TurningCircleRadius = DefaultTurningCircleRadius * VehiclePawn->GetVehicleSpeedMph() / 10.0f;

	bool bShouldTurnAround = TargetDistance < TurningCircleRadius;

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, VeryVerbose,
		TEXT("%s-%s: ShouldReverseThrottleAndSteering - Result=%s; bTurningAround=%s; ForwardFactor=%f; TurnAroundFactor=%f; TargetDistance=%f; TurningCircleRadius=%f"),
		*GetName(),
		*VehiclePawn->GetName(),
		LoggingUtils::GetBoolString(bShouldTurnAround),
		LoggingUtils::GetBoolString(bTurningAround),
		ForwardFactor,
		TurnAroundThreshold,
		TargetDistance,
		TurningCircleRadius
	);

	return bShouldTurnAround;
}

void UAA_AIVehicleControlComponent::CalculateReverseThrottleAndSteering(const FVector& DestinationDelta) const
{
	check(VehiclePawn);

	const float ThrottleValue = ReverseThrottleValue;
	const float TargetYawDelta = GetTargetSteeringYawAngle(DestinationDelta);

	// Reverse the steering direction
	const float SteeringValue = !FMath::IsNearlyZero(TargetYawDelta) ? (TargetYawDelta > 0 ? -1.0f : 1.0f) : 0.0f;

	SetSpeedControls(ThrottleValue);
	VehiclePawn->SetSteering(SteeringValue);

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, VeryVerbose,
		TEXT("%s-%s: ReverseThrottleAndSteering - ThrottleValue=%f; SteeringValue=%f; CurrentMaxSteeringAngle=%f; TargetYawDeltaAngle=%f"),
		*GetName(),
		*VehiclePawn->GetName(),
		ThrottleValue,
		SteeringValue,
		CurrentMaxSteeringAngle,
		TargetYawDelta
	);
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

void UAA_AIVehicleControlComponent::SetSpeedControls(float ThrottleValue) const
{
	auto MovementComponent = VehiclePawn->GetVehicleMovementComponent();
	check(MovementComponent);

	if (ThrottleValue >= 0)
	{
		if (MovementComponent->GetTargetGear() == -1 && VehiclePawn->GetVehicleSpeedMph() > -ReverseSpeedThresholdMph)
		{
			MovementComponent->SetTargetGear(1, true);
		}

		VehiclePawn->SetHandbrake(false);
		VehiclePawn->SetThrottle(ThrottleValue);
		VehiclePawn->SetBrake(0);
	}
	else if(VehiclePawn->GetVehicleSpeedMph() > ReverseSpeedThresholdMph)
	{
		VehiclePawn->SetThrottle(0);
		if (bTurningAround)
		{
			VehiclePawn->SetHandbrake(true);
			VehiclePawn->SetBrake(1.0f);
		}
		else
		{
			VehiclePawn->SetBrake(-ThrottleValue);
		}
	}
	// Reverse
	else
	{
		MovementComponent->SetTargetGear(-1, true);

		VehiclePawn->SetThrottle(-ThrottleValue);
		VehiclePawn->SetBrake(0);
		VehiclePawn->SetHandbrake(false);
	}
}

bool UAA_AIVehicleControlComponent::HasReachedTarget() const
{
	check(VehiclePawn);

	return FVector::Distance(VehiclePawn->GetFrontWorldLocation(), CurrentMovementTarget) < TargetReachedRadius;
}

void UAA_AIVehicleControlComponent::CheckIfReachedTarget()
{
	if (bTargetReached || !HasReachedTarget())
	{
		return;
	}

	bTargetReached = true;

	check(VehiclePawn);

	UE_VLOG_EVENT_WITH_DATA(GetOwner(), EventVehicleTargetReached, *LoggingUtils::GetName(VehiclePawn));

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

#if ENABLE_VISUAL_LOG

void UAA_AIVehicleControlComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("AI Vehicle Control");

	Category.Add(TEXT("DesiredSpeedMph"), FString::Printf(TEXT("%.1f"), DesiredSpeedMph));
	Category.Add(TEXT("Turning Around"), LoggingUtils::GetBoolString(bTurningAround));
	Category.Add(TEXT("CurrentMovementTarget"), bTargetSet ? *CurrentMovementTarget.ToCompactString() : TEXT("N/A"));
	Category.Add(TEXT("TargetReached"), bTargetSet ? LoggingUtils::GetBoolString(bTargetReached) : TEXT("N/A"));

	Snapshot->Status.Add(Category);
}

#endif
