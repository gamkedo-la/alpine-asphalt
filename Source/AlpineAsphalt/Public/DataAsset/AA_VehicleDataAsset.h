// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "AA_VehicleDataAsset.generated.h"

/**
 * Data used to instantiate a Wheeled Vehicle Pawn
 */
UCLASS(BlueprintType)
class ALPINEASPHALT_API UAA_VehicleDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty( FPropertyChangedEvent & PropertyChangedEvent ) override;

	DECLARE_EVENT(UAA_VehicleDataAsset , FOnChanged);
	FOnChanged OnValueChanged;
#endif
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	USkeletalMesh* VehicleMesh;

	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	UAnimBlueprint* AnimationInstance;

	UPROPERTY(EditAnywhere, Category = WheelSetup)
	bool bSuspensionEnabled = true;

	UPROPERTY(EditAnywhere, Category = WheelSetup)
	bool bWheelFrictionEnabled = true;

	UPROPERTY(EditAnywhere, Category = WheelSetup)
	bool bLegacyWheelFrictionPosition = true;

	UPROPERTY(EditAnywhere, Category = WheelSetup)
	FCollisionResponseContainer WheelTraceCollisionResponses;
	
	/** Wheels to create */
	UPROPERTY(EditAnywhere, Category = WheelSetup)
	TArray<FChaosWheelSetup> WheelSetups;

	UPROPERTY(EditAnywhere, Category = MechanicalSetup)
	bool bMechanicalSimEnabled = true;

	/** Engine */
	UPROPERTY(EditAnywhere, Category = MechanicalSetup, meta = (EditCondition = "bMechanicalSimEnabled"))
	FVehicleEngineConfig EngineSetup;

	/** Differential */
	UPROPERTY(EditAnywhere, Category = MechanicalSetup, meta = (EditCondition = "bMechanicalSimEnabled"))
	FVehicleDifferentialConfig DifferentialSetup;

	/** Transmission data */
	UPROPERTY(EditAnywhere, Category = MechanicalSetup, meta = (EditCondition = "bMechanicalSimEnabled"))
	FVehicleTransmissionConfig TransmissionSetup;

	/** Transmission data */
	UPROPERTY(EditAnywhere, Category = SteeringSetup)
	FVehicleSteeringConfig SteeringSetup;
	
};
