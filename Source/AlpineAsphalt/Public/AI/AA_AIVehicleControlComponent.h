// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "AI/AA_AIRacerEvents.h"
#include "Interface/AA_BaseRewindable.h"

#include "AA_AIVehicleControlComponent.generated.h"

class AAA_WheeledVehiclePawn;

struct FAA_AIVehicleControlComponentSnapshotData
{
	FVector CurrentMovementTarget{ EForceInit::ForceInitToZero };
	float DesiredSpeedMph{};

	bool bTurningAround{};
	bool bTargetReached{};
	bool bTargetSet{};
	bool bTargetStartedBehind{};
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALPINEASPHALT_API UAA_AIVehicleControlComponent : public UActorComponent, public TAA_BaseRewindable<FAA_AIVehicleControlComponentSnapshotData>
{
	GENERATED_BODY()

protected:
	using FSnapshotData = FAA_AIVehicleControlComponentSnapshotData;

public:	
	UAA_AIVehicleControlComponent();

	void SetVehiclePawn(AAA_WheeledVehiclePawn* Pawn);
	UFUNCTION()
	void OnVehicleTargetUpdated(AAA_WheeledVehiclePawn* TheVehiclePawn, const FVector& MovementTarget, float NewDesiredSpeedMph);

	UFUNCTION(BlueprintCallable)
	void SetDesiredSpeedMph(float SpeedMph);

	UFUNCTION(BlueprintCallable)
	void SetMovementTarget(const FVector& MovementTarget);

	UFUNCTION(BlueprintPure)
	float GetDesiredSpeedMph() const;

	UFUNCTION(BlueprintPure)
	FVector GetMovementTarget() const;

	UFUNCTION(BlueprintPure)
	bool IsTargetReached() const;

	UFUNCTION(BlueprintPure)
	bool IsTargetSet() const;

#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisualLogEntry* Snapshot) const;
#endif // ENABLE_VISUAL_LOG

protected:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Inherited via TAA_BaseRewindable
	FSnapshotData CaptureSnapshot() const override;
	void RestoreFromSnapshot(const FSnapshotData& InSnapshotData, float InRewindTime) override;

private:

	void DoTick();

	void CalculateThrottle() const;
	float SmoothThrottle(float CurrentSpeed) const;
	void SetSpeedControls(float ThrottleValue) const;

	void CalculateSteering(const FVector& DestinationDelta) const;

	bool ShouldReverseThrottleAndSteering(const FVector& DestinationDelta) const;
	void CalculateReverseThrottleAndSteering(const FVector& DestinationDelta) const;

	float GetTargetSteeringYawAngle(const FVector& DestinationDelta) const;

	bool HasReachedTarget(float* OutDistance = nullptr) const;
	void CheckIfReachedTarget();
	bool IsTargetBehind() const;
	bool IsTargetUnreachable(float CurrentDistance) const;

	float CalculateNextTickInterval() const;


	// Inherited via TAA_BaseRewindable
	virtual UObject* AsUObject() override { return this; }

public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	mutable FOnVehicleReachedTarget OnVehicleReachedTarget;

	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	mutable FOnVehicleTargetUnreachable OnVehicleTargetUnreachable;

private:
	UPROPERTY(Transient)
	TObjectPtr<AAA_WheeledVehiclePawn> VehiclePawn{};

	/*
	* Delta radius for when the target reached event will be generated.
	*/
	UPROPERTY(EditAnywhere)
	float TargetReachedRadius{ 250.0f };


	/*
	* Distance where we report that the current target is unreachable.
	*/
	UPROPERTY(EditAnywhere)
	float TargetUnreachableDistance{ 50 * 100.0f };

	/**
	*  DeltaZ where we report that the current target is unreachable regardless of distance.
	*/
	UPROPERTY(EditAnywhere)
	float TargetUnreachableDeltaZ{ 5 * 100.0f };

	// TODO: Will calculate this from wheel data in the movement component and also changes based on velocity from steering curve
	UPROPERTY(EditAnywhere)
	float CurrentMaxSteeringAngle{ 45.0f };

	// TODO: Will calculate this from vehicle data in the movement component
	UPROPERTY(EditAnywhere)
	float DefaultTurningCircleRadius{ 1200.0f };

	UPROPERTY(EditAnywhere)
	float TurningCircleRadiusSpeedMphBase{ 10.0f };

	/*
	* Desired speed in mph
	*/
	UPROPERTY(EditAnywhere)
	float DesiredSpeedMph{ 40.0f };

	UPROPERTY(EditAnywhere)
	float DampeningDistance{ 50.0f };

	UPROPERTY(EditAnywhere)
	float DampeningExponent{ 2.0f };

	UPROPERTY(EditAnywhere)
	float FullThrottleSpeedThresholdMph{ 50.0f };

	UPROPERTY(EditAnywhere)
	float FullThrottleExponent{ 1 / 3.0f };

	UPROPERTY(EditAnywhere)
	float DeltaSpeedExponent{ 3.0f };

	UPROPERTY(EditAnywhere)
	float RawFactorSwitchoverThreshold{ 0.1f };

	UPROPERTY(Transient, VisibleInstanceOnly)
	bool bTurningAround{ false };

	UPROPERTY(EditAnywhere)
	float TurnAroundCosineThreshold{ 0.0f };

	UPROPERTY(EditAnywhere)
	float ContinueTurningAroundCosineThreshold{ 0.25f };

	UPROPERTY(EditAnywhere)
	float ReverseThrottleValue{ -1.0f / 3 };

	UPROPERTY(EditAnywhere)
	float ReverseSpeedThresholdMph{ 5.0f };

	UPROPERTY(EditAnywhere)
	float TargetBehindDotThreshold{ 0.1f };

	/*
	* Current movement target.
	*/
	UPROPERTY(Transient, EditAnywhere)
	FVector CurrentMovementTarget{ EForceInit::ForceInitToZero };

	UPROPERTY(Transient, VisibleAnywhere)
	bool bTargetReached{ };

	bool bTargetSet{};
	bool bTargetStartedBehind{};

	UPROPERTY(EditDefaultsOnly, Category = Tick)
	bool bVariableTick{ true };

	UPROPERTY(EditDefaultsOnly, Category = Tick)
	float MinTickInterval { 1 / 30.0f };

	UPROPERTY(EditDefaultsOnly, Category = Tick)
	float MaxTickInterval { 1.0f };

	UPROPERTY(EditDefaultsOnly, Category = Tick)
	float TargetTickCompletionFraction{ 1 / 3.0f };
};


#pragma region Inline Definitions

inline float UAA_AIVehicleControlComponent::GetDesiredSpeedMph() const
{
	return DesiredSpeedMph;
}

inline FVector UAA_AIVehicleControlComponent::GetMovementTarget() const
{
	return CurrentMovementTarget;
}

inline bool UAA_AIVehicleControlComponent::IsTargetReached() const
{
	return bTargetReached;
}

inline bool UAA_AIVehicleControlComponent::IsTargetSet() const
{
	return bTargetSet;
}

#pragma endregion Inline Definitions
