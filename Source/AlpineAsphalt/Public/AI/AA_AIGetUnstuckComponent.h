// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AA_AIRacerEvents.h"
#include "Interface/AA_BaseRewindable.h"

#include "AA_AIGetUnstuckComponent.generated.h"

class IAA_RacerContextProvider;
class AAA_WheeledVehiclePawn;

namespace AA_AIGetUnstuckComponent
{
	struct FStuckState
	{
		FVector Position{ EForceInit::ForceInitToZero };
		int8 ThrottleSign : 1 {};
	};

	struct FSnapshotData
	{
		constexpr static uint32 MaxSnapshotBufferSize = 32;

		TArray<FStuckState,TInlineAllocator<MaxSnapshotBufferSize>> Positions{};

		int32 ConsecutiveStuckCount{};
		uint32 NextBufferIndex{};
		uint32 NumSamples{};

		float LastStuckTime{ -1.0f };
		bool bHasStarted{};
	};
}
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALPINEASPHALT_API UAA_AIGetUnstuckComponent : public UActorComponent, public TAA_BaseRewindable<AA_AIGetUnstuckComponent::FSnapshotData>
{
	GENERATED_BODY()

public:
	UAA_AIGetUnstuckComponent();

#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisualLogEntry* Snapshot) const;
#endif // ENABLE_VISUAL_LOG

public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	mutable FOnVehicleStuck OnVehicleStuck {};


protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Inherited via TAA_BaseRewindable
	virtual AA_AIGetUnstuckComponent::FSnapshotData CaptureSnapshot() const override;
	virtual void RestoreFromSnapshot(const AA_AIGetUnstuckComponent::FSnapshotData& InSnapshotData, float InRewindTime) override;

private:
	void ResetBuffer();
	FVector CalculateIdealSeekPosition(const AAA_WheeledVehiclePawn& VehiclePawn) const;

	bool IsPermanentlyStuck() const;

	bool IsVehicleFlippedOver() const;

	// Inherited via TAA_BaseRewindable
	virtual UObject* AsUObject() override { return this; }
private:

	IAA_RacerContextProvider* RacerContextProvider{};
	TUniquePtr<TCircularBuffer<AA_AIGetUnstuckComponent::FStuckState>> PositionsPtr{};

	UPROPERTY(Category = "Detection", EditAnywhere)
	float MinStuckTime{ 3.0f };

	UPROPERTY(Category = "Detection", EditAnywhere)
	float MinAverageSpeed{ 100.0f };

	UPROPERTY(Category = "Movement", EditAnywhere)
	float UnstuckSeekOffset{ 1000.0f };

	UPROPERTY(Category = "Movement", EditAnywhere)
	int32 MaxOffsets{ 5 };

	/**
	* Absolute value of the pitch angle where we detect the car is probably flipped over.
	*/
	UPROPERTY(Category = "Movement", EditDefaultsOnly)
	float MinFlippedOverPitchDetectionAngle{ 80.0f };

	int32 ConsecutiveStuckCount{};
	float LastStuckTime{ -1.0f };

	uint32 NextBufferIndex{};
	uint32 MinNumSamples{};
	uint32 NumSamples{};

	// Don't trigger stuck at start - wait for some movement initially
	// TODO: Maybe there is an event we can subscribe to to "turn on the functionality"
	bool bHasStarted{};
};
