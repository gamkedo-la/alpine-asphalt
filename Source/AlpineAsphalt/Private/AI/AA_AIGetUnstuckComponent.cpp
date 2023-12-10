// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AA_AIGetUnstuckComponent.h"

#include "Logging/AlpineAsphaltLogger.h"
#include "Logging/LoggingUtils.h"
#include "Util/UnitConversions.h"
#include "VisualLogger/VisualLogger.h"
#include "AI/AA_AIRacerContext.h"
#include "AI/AA_RacerContextProvider.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/AA_ChaosWheeledVehicleMovementComponent.h"
#include "Pawn/AA_WheeledVehiclePawn.h"

#include "Kismet/KismetMathLibrary.h"

using namespace AA;
using namespace AA_AIGetUnstuckComponent;

DEFINE_VLOG_EVENT(EventAIVehicleStuck, Display, "AI Vehicle Stuck")

// Sets default values for this component's properties
UAA_AIGetUnstuckComponent::UAA_AIGetUnstuckComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.2f;
}

#if ENABLE_VISUAL_LOG
void UAA_AIGetUnstuckComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{
	FVisualLogStatusCategory Category;
	Category.Category = TEXT("Get Unstuck Component");

	Category.Add(TEXT("MinStuckTime"), FString::Printf(TEXT("%.1fs"), MinStuckTime));
	Category.Add(TEXT("MinAverageSpeed"), FString::Printf(TEXT("%.1f cm/s"), MinAverageSpeed));
	Category.Add(TEXT("UnstuckSeekOffset"), FString::Printf(TEXT("%.1f cm"), UnstuckSeekOffset));
	Category.Add(TEXT("MaxOffsets"), FString::Printf(TEXT("%d"), MaxOffsets));
	Category.Add(TEXT("ConsecutiveStuckCount"), FString::Printf(TEXT("%d"), ConsecutiveStuckCount));
	Category.Add(TEXT("LastStuckTime"), FString::Printf(TEXT("%.1fs"), LastStuckTime));
	Category.Add(TEXT("NextBufferIndex"), FString::Printf(TEXT("%d"), NextBufferIndex));
	Category.Add(TEXT("NumSamples"), FString::Printf(TEXT("%d"), NextBufferIndex));
	Category.Add(TEXT("MinNumSamples"), FString::Printf(TEXT("%d"), NextBufferIndex));
	Category.Add(TEXT("BufferCapacity"), PositionsPtr ? FString::Printf(TEXT("%d"), PositionsPtr->Capacity()) : TEXT("N/A"));
	Category.Add(TEXT("SufficientSamples"), FString::Printf(TEXT("%s"), LoggingUtils::GetBoolString(NumSamples >= MinNumSamples)));
	Category.Add(TEXT("HasStarted"), FString::Printf(TEXT("%s"), LoggingUtils::GetBoolString(bHasStarted)));

	Snapshot->Status.Add(Category);
}
#endif

void UAA_AIGetUnstuckComponent::BeginPlay()
{
	Super::BeginPlay();

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log, TEXT("BeginPlay: MinStuckTime=%fs"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), MinStuckTime);

	RacerContextProvider = Cast<IAA_RacerContextProvider>(GetOwner());

	if (!RacerContextProvider)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Error,
			TEXT("%s-%s: BeginPlay - Owner does not implement IAA_RacerContextProvider"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));

		PrimaryComponentTick.SetTickFunctionEnable(false);

		return;
	}

	// Size the buffer to hold enough entries for the min stuck time
	// Note that for TCircularBuffer the actual capacity is next power of 2 so must note the MinNumSamples
	MinNumSamples = MinStuckTime / PrimaryComponentTick.TickInterval;

	PositionsPtr = MakeUnique<TCircularBuffer<FStuckState>>(MinNumSamples);

	ensureMsgf(PositionsPtr->Capacity() <= AA_AIGetUnstuckComponent::FSnapshotData::MaxSnapshotBufferSize, TEXT("MinStuckTime=%fs / TickInterval=%fs = %d > MaxSnapshotBufferSize=%d"),
		MinStuckTime, PrimaryComponentTick.TickInterval, AA_AIGetUnstuckComponent::FSnapshotData::MaxSnapshotBufferSize);

	RegisterRewindable(ERestoreTiming::Resume);

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log, TEXT("%s-%s: BeginPlay: Buffer Capacity=%d"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), PositionsPtr ? PositionsPtr->Capacity() : 0);
}


void UAA_AIGetUnstuckComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!PositionsPtr || !RacerContextProvider)
	{
		return;
	}

	const auto& Context = RacerContextProvider->GetRacerContext();
	auto VehiclePawn = Context.VehiclePawn;

	if (!VehiclePawn)
	{
		return;
	}

	auto MovementComponent = VehiclePawn->GetVehicleMovementComponent();
	if (!MovementComponent)
	{
		return;
	}

	auto& Positions = *PositionsPtr;

	const auto CurrentBufferIndex = NextBufferIndex;

	// Sample the position
	const auto& CurrentPosition = VehiclePawn->GetFrontWorldLocation();
	
	FStuckState CurrentState =
	{
		.Position = CurrentPosition,
		.ThrottleSign = MovementComponent->GetThrottleInput() >= 0
	};

	++NumSamples;
	Positions[CurrentBufferIndex] = CurrentState;
	NextBufferIndex = Positions.GetNextIndex(CurrentBufferIndex);

	if (NumSamples < MinNumSamples)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
			TEXT("%s-%s: TickComponent - Insufficient Samples %d < %d"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), NumSamples, MinNumSamples);
		return;
	}

	// Difference between current position and the oldest one which is the next one we will overwrite since we wrap around once the capacity is reached
	// Remember that the TCircularBuffer Capacity is next power of 2 and not necessarily the MinNumSamples
	const auto OldestBufferIndex = NumSamples <= Positions.Capacity() ? 0 : NextBufferIndex;
	const auto& OldestState = Positions[OldestBufferIndex];

	// if we just changed throttle sign then cooldown
	if (CurrentState.ThrottleSign != OldestState.ThrottleSign)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
			TEXT("%s-%s: TickComponent - Changed throttle sign:  %d -> %d"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), OldestState.ThrottleSign, CurrentState.ThrottleSign);

		ResetBuffer();
		return;
	}

	const auto Displacement = CurrentPosition - OldestState.Position;
	const auto AverageSpeedSq = Displacement.SizeSquared() / MinStuckTime;

	if (AverageSpeedSq >= FMath::Square(MinAverageSpeed))
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
			TEXT("%s-%s: TickComponent - Not Stuck - AverageSpeed=%fcm/s >= MinAverageSpeed=%fcm/s"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), FMath::Sqrt(AverageSpeedSq), MinAverageSpeed);
		// We got some movement, we can now officially "turn on" the functionality
		bHasStarted = true;
		return;
	}

	if (!bHasStarted)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
			TEXT("%s-%s: TickComponent - Skipping Stuck as no movement detected yet"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
		ResetBuffer();
		return;
	}

	const auto CurrentTimeSeconds = GetWorld()->GetTimeSeconds();
	if (CurrentTimeSeconds - LastStuckTime > 2 * MinStuckTime)
	{
		// new stuck event - reset counter
		ConsecutiveStuckCount = 1;
	}
	else
	{
		++ConsecutiveStuckCount;
	}

	LastStuckTime = CurrentTimeSeconds;

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
		TEXT("%s-%s: TickComponent - Stuck (%d x) - AverageVelocity=%fcm/s < MinAverageSpeed=%fcm/s"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), ConsecutiveStuckCount, FMath::Sqrt(AverageSpeedSq), MinAverageSpeed);

	// We are stuck, make sure we cool down by resetting the buffer after the event is fired
	ResetBuffer();

	UE_VLOG_EVENT_WITH_DATA(GetOwner(), EventAIVehicleStuck, *LoggingUtils::GetName(VehiclePawn));

	const auto& IdealSeekPosition = CalculateIdealSeekPosition(*VehiclePawn);

	UE_VLOG_CYLINDER(GetOwner(), LogAlpineAsphalt, Display, IdealSeekPosition, IdealSeekPosition + FVector{ 0,0, 100 }, 50.0f, FColor::Blue,
		TEXT("%s: Unstuck Seek Target"), *VehiclePawn->GetName());

	const bool bPermanentlyStuck = IsPermanentlyStuck();
	OnVehicleStuck.Broadcast(VehiclePawn, IdealSeekPosition, bPermanentlyStuck);

	if (bPermanentlyStuck)
	{
		ConsecutiveStuckCount = 0;
	}
}

void UAA_AIGetUnstuckComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UnregisterRewindable();
}

AA_AIGetUnstuckComponent::FSnapshotData UAA_AIGetUnstuckComponent::CaptureSnapshot() const
{
	AA_AIGetUnstuckComponent::FSnapshotData Data
	{
		.ConsecutiveStuckCount = ConsecutiveStuckCount,
		.NextBufferIndex = NextBufferIndex,
		.NumSamples = NumSamples,
		.LastStuckTime = LastStuckTime,
		.bHasStarted = bHasStarted
	};

	if (PositionsPtr)
	{
		auto& SnapshotPositions = Data.Positions;
		const auto& Positions = *PositionsPtr;

		for (int32 i = 0, Len = NumSamples >= Positions.Capacity() ? Positions.Capacity() : NextBufferIndex; i < Len; ++i)
		{
			SnapshotPositions.Add(Positions[i]);
		}
	}

	return Data;
}

void UAA_AIGetUnstuckComponent::RestoreFromSnapshot(const AA_AIGetUnstuckComponent::FSnapshotData& InSnapshotData, float InRewindTime)
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
		TEXT("%s-%s: RestoreFromSnapshot: InRewindTime=%f"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), InRewindTime);

	ConsecutiveStuckCount = InSnapshotData.ConsecutiveStuckCount;
	NextBufferIndex = InSnapshotData.NextBufferIndex;
	// need to increase the time because the reference point for what is "Now" has changed
	// We went "back in time" gameplay wise but GetWorldTimeSeconds() is still where it was before
	LastStuckTime = InSnapshotData.LastStuckTime + InRewindTime; 
	NumSamples = InSnapshotData.NumSamples;
	bHasStarted = InSnapshotData.bHasStarted;

	if (!PositionsPtr)
	{
		return;
	}

	const auto& SnapshotPositions = InSnapshotData.Positions;
	auto& Positions = *PositionsPtr;

	for (int32 i = 0; i < SnapshotPositions.Num(); ++i)
	{
		Positions[i] = SnapshotPositions[i];
	}
}

void UAA_AIGetUnstuckComponent::ResetBuffer()
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
		TEXT("%s-%s: ResetBuffer"),
		*GetName(), *LoggingUtils::GetName(GetOwner()));

	NextBufferIndex = NumSamples = 0;
}

FVector UAA_AIGetUnstuckComponent::CalculateIdealSeekPosition(const AAA_WheeledVehiclePawn& VehiclePawn) const
{
	// If trying to throttle up go backwards; otherwise, go forwards
	auto MovementComponent = VehiclePawn.GetVehicleMovementComponent();
	check(MovementComponent);

	bool bSeekForward = MovementComponent->GetThrottleInput() < 0;

	const auto AdjustedUnstuckSeekOffset = UnstuckSeekOffset * ConsecutiveStuckCount;
	
	if (bSeekForward)
	{
		return VehiclePawn.GetFrontWorldLocation() + VehiclePawn.GetActorForwardVector() * AdjustedUnstuckSeekOffset;
	}

	return VehiclePawn.GetBackWorldLocation() - VehiclePawn.GetActorForwardVector() * AdjustedUnstuckSeekOffset;
}

bool UAA_AIGetUnstuckComponent::IsPermanentlyStuck() const
{
	if (ConsecutiveStuckCount == MaxOffsets - 1)
	{
		return true;
	}

	return IsVehicleFlippedOver();
}

bool UAA_AIGetUnstuckComponent::IsVehicleFlippedOver() const
{
	check(RacerContextProvider);
	const auto& Context = RacerContextProvider->GetRacerContext();
	auto VehiclePawn = Context.VehiclePawn;
	check(VehiclePawn);

	auto World = GetWorld();
	check(World);

	// Find elevation at TargetLocation
	FCollisionQueryParams CollisionQueryParams;
	CollisionQueryParams.AddIgnoredActor(VehiclePawn);

	const auto& TargetLocation = VehiclePawn->GetActorLocation();

	const auto TraceStart = TargetLocation + FVector(0, 0, VehiclePawn->GetVehicleHeight() * 0.5f);
	const auto TraceEnd = TargetLocation - FVector(0, 0, 1000);

	FHitResult HitResult;

	if (!World->LineTraceSingleByChannel(
		HitResult,
		TraceStart,
		TraceEnd,
		ECollisionChannel::ECC_Visibility,
		CollisionQueryParams))
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Warning,
			TEXT("%s-%s: IsVehicleFlippedOver: Could not determine ground location"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));

		return false;
	}

	const FVector& VehicleUpVector = VehiclePawn->GetActorUpVector();

	const auto& WorldUpVector = HitResult.Normal;
	const auto DotProduct = VehicleUpVector | WorldUpVector;

	const float Angle = FMath::Abs(FMath::RadiansToDegrees(FMath::Acos(DotProduct)));
	const bool bIsFlippedOver = Angle >= MinFlippedOverPitchDetectionAngle;

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
		TEXT("%s-%s: IsVehicleFlippedOver: %s: Angle=%f"),
		*GetName(), *LoggingUtils::GetName(GetOwner()), LoggingUtils::GetBoolString(bIsFlippedOver), Angle);

	UE_VLOG_ARROW(GetOwner(), LogAlpineAsphalt, Log, HitResult.Location, HitResult.Location + 100.0f * WorldUpVector, FColor::Red, TEXT("World Up"));
	UE_VLOG_ARROW(GetOwner(), LogAlpineAsphalt, Log, HitResult.Location, HitResult.Location + 100.0f * VehicleUpVector, FColor::Red, TEXT("Actor Up"));

	return bIsFlippedOver;
}
