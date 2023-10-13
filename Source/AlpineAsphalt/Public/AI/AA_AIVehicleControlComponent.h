// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "AI/AA_AIRacerEvents.h"

#include "AA_AIVehicleControlComponent.generated.h"

class AAA_WheeledVehiclePawn;


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALPINEASPHALT_API UAA_AIVehicleControlComponent : public UActorComponent
{
	GENERATED_BODY()

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

private:
	void CalculateThrottle() const;
	float SmoothThrottle(float CurrentSpeed) const;
	void SetSpeedControls(float ThrottleValue) const;

	void CalculateSteering(const FVector& DestinationDelta) const;

	bool ShouldReverseThrottleAndSteering(const FVector& DestinationDelta) const;
	void CalculateReverseThrottleAndSteering(const FVector& DestinationDelta) const;

	float GetTargetSteeringYawAngle(const FVector& DestinationDelta) const;

	bool HasReachedTarget() const;
	void CheckIfReachedTarget();
	bool IsTargetBehind() const;

public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	mutable FOnVehicleReachedTarget OnVehicleReachedTarget;

private:
	UPROPERTY(Transient)
	TObjectPtr<AAA_WheeledVehiclePawn> VehiclePawn{};

	/*
	* Delta radius for when the target reached event will be generated.
	*/
	UPROPERTY(EditAnywhere)
	float TargetReachedRadius{ 250.0f };

	// TODO: Will calculate this from wheel data in the movement component and also changes based on velocity from steering curve
	UPROPERTY(EditAnywhere)
	float CurrentMaxSteeringAngle{ 45.0f };

	// TODO: Will calculate this from vehicle data in the movement component
	UPROPERTY(EditAnywhere)
	float DefaultTurningCircleRadius{ 1200.0f };

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

	UPROPERTY(VisibleAnywhere)
	bool bTurningAround{ false };

	UPROPERTY(EditAnywhere)
	float TurnAroundCosineThreshold{ 0.0f };

	UPROPERTY(EditAnywhere)
	float ContinueTurningAroundCosineThreshold{ 0.25f };

	UPROPERTY(EditAnywhere)
	float ReverseThrottleValue{ -1.0f / 3 };

	UPROPERTY(EditAnywhere)
	float ReverseSpeedThresholdMph{ 5.0f };

	/*
	* Current movement target.
	*/
	UPROPERTY(EditAnywhere)
	FVector CurrentMovementTarget{ EForceInit::ForceInitToZero };

	UPROPERTY(VisibleAnywhere)
	bool bTargetReached{ };

	bool bTargetSet{};
	bool bTargetStartedBehind{};
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
