// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AA_Rewindable_CPP.h"
#include "SnapshotData.h"
#include "AA_WheeledVehiclePawn_CPP.generated.h"

struct FInputActionValue;
//Define Log for Vehicle
DECLARE_LOG_CATEGORY_EXTERN(Vehicle, Log, All);
/**
 * 
 */
UCLASS()
class ALPINEASPHALT_API AAA_WheeledVehiclePawn_CPP : public APawn, public IAA_Rewindable_CPP
{
	GENERATED_BODY()

private:
	/**  The main skeletal mesh associated with this Vehicle */
	UPROPERTY(Category = Vehicle, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class USkeletalMeshComponent> Mesh;

	/** vehicle simulation component */
	UPROPERTY(Category = Vehicle, VisibleDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<class UChaosWheeledVehicleMovementComponent> VehicleMovementComponent;

	UPROPERTY(EditAnywhere,BlueprintReadOnly,meta= (AllowPrivateAccess = "true"))
	TObjectPtr<class UAA_VehicleDataAsset_CPP> VehicleData;
	
public:
	/** Constructor*/
	AAA_WheeledVehiclePawn_CPP(const class FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	/** Name of the MeshComponent. Use this name if you want to prevent creation of the component (with ObjectInitializer.DoNotCreateDefaultSubobject). */
	static FName VehicleMeshComponentName;

	/** Name of the VehicleMovement. Use this name if you want to use a different class (with ObjectInitializer.SetDefaultSubobjectClass). */
	static FName VehicleMovementComponentName;

	/** Util to get the wheeled vehicle movement component */
	UChaosWheeledVehicleMovementComponent* GetVehicleMovementComponent() const;

	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void SetHandbrake(bool bEnabled);

	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void SetBrake(const float BrakeValue);

	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void ResetVehicle();

	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void ShiftUp();

	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void ShiftDown();

	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void SetSteering(float SteeringInput);

	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void SetThrottle(float Throttle);

	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void ToggleCamera();

	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void CameraLook(FVector2D Input);

#if WITH_EDITOR
	virtual void PostEditChangeProperty( FPropertyChangedEvent & PropertyChangedEvent ) override;
#endif

	//~ Begin AActor Interface
	virtual void DisplayDebug(class UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;

	UFUNCTION(BlueprintCallable)
	void SetVehicleData(UAA_VehicleDataAsset_CPP* NewVehicleData);

	/** Rewind Functions **/
	virtual void SetRewindTime(float Time) override;
	virtual void PauseRecordingSnapshots() override;
	virtual void ResumeRecordingSnapshots() override;
	void RecordSnapshot();
	/** Rewind Properties **/
	FTimerHandle RecordingSnapshotTimerHandle;
	TArray<FWheeledSnaphotData> SnapshotData;

	//Cached property of Rewind Subsystem
	int MaxSnapshots = 0;
	//Cached Time that should match RewindSubsystem
	float RewindTime = 0;
	//Cached property of Rewind Subsystem
	float RewindResolution = 0.f;
};
