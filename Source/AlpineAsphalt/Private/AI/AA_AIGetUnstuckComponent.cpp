// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AA_AIGetUnstuckComponent.h"

#include "Logging/AlpineAsphaltLogger.h"
#include "Logging/LoggingUtils.h"
#include "Util/UnitConversions.h"
#include "VisualLogger/VisualLogger.h"
#include "AI/AA_AIRacerContext.h"
#include "AI/AA_RacerContextProvider.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Pawn/AA_WheeledVehiclePawn.h"

#include "Kismet/KismetMathLibrary.h"

using namespace AA;

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
	Category.Add(TEXT("SufficientSamples"), FString::Printf(TEXT("%s"), LoggingUtils::GetBoolString(bSufficientSamples)));
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
	PositionsPtr = MakeUnique<TCircularBuffer<FStuckState>>(FMath::CeilToInt(MinStuckTime / PrimaryComponentTick.TickInterval));

	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log, TEXT("BeginPlay: Buffer Capacity=%d"),
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

	Positions[CurrentBufferIndex] = CurrentState;
	NextBufferIndex = Positions.GetNextIndex(CurrentBufferIndex);

	if (!bSufficientSamples && CurrentBufferIndex == Positions.Capacity() - 1)
	{
		bSufficientSamples = true;
	}

	if (!bSufficientSamples)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Verbose,
			TEXT("%s-%s: TickComponent - Insufficient Samples %d < %d"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), CurrentBufferIndex, Positions.Capacity());
		return;
	}

	// Difference between current position and the oldest one which is the next one we will overwrite since we wrap around once the capacity is reached
	const auto& OldestState = Positions[NextBufferIndex];

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
	if (CurrentTimeSeconds - LastStuckTime > 2 * MinStuckTime || ConsecutiveStuckCount >= MaxOffsets)
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

	OnVehicleStuck.Broadcast(VehiclePawn, IdealSeekPosition);
}

void UAA_AIGetUnstuckComponent::ResetBuffer()
{
	UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Log,
		TEXT("%s-%s: ResetBuffer"),
		*GetName(), *LoggingUtils::GetName(GetOwner()));

	NextBufferIndex = 0;
	bSufficientSamples = false;
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
