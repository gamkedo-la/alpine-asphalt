// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AA_AIVehicleControlComponent.generated.h"

class AAA_WheeledVehiclePawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnVehicleReachedTarget, AAA_WheeledVehiclePawn*, VehiclePawn, const FVector&, Target);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALPINEASPHALT_API UAA_AIVehicleControlComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAA_AIVehicleControlComponent();

	void SetVehiclePawn(AAA_WheeledVehiclePawn* Pawn);

	UFUNCTION(BlueprintCallable)
	void SetDesiredSpeedMph(float SpeedMph);

	UFUNCTION(BlueprintCallable)
	void SetMovementTarget(const FVector& MovementTarget);


protected:
	virtual void BeginPlay() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void CalculateThrottle() const;
	float SmoothThrottle(float CurrentSpeed) const;

	void CalculateSteering() const;
	float GetTargetSteeringYawAngle(const FVector& DestinationDelta) const;

	bool HasReachedTarget() const;
	void CheckIfReachedTarget();

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
	float TargetReachedRadius{ 200.0f };

	// TODO: Will calculate this from wheel data in the movement component and also changes based on velocity from steering curve
	UPROPERTY(EditAnywhere)
	float CurrentMaxSteeringAngle{ 45.0f };

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

	/*
	* Current movement target.
	*/
	UPROPERTY(EditAnywhere)
	FVector CurrentMovementTarget{ EForceInit::ForceInitToZero };

	UPROPERTY(VisibleAnywhere)
	bool bTargetReached{ };

	bool bTargetSet{};
};

#pragma region Inline Definitions

#pragma endregion Inline Definitions
