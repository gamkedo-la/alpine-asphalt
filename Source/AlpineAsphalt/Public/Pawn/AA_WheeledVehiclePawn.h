// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interface/AA_RewindableInterface.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "SnapshotData.h"

#include "AA_WheeledVehiclePawn.generated.h"

struct FInputActionValue;
//Define Log for Vehicle
DECLARE_LOG_CATEGORY_EXTERN(LogAAVehicle, Log, All);
/**
 * 
 */
UCLASS()
class ALPINEASPHALT_API AAA_WheeledVehiclePawn : public APawn, public IAA_RewindableInterface, public IVisualLoggerDebugSnapshotInterface
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
	TObjectPtr<class UAA_VehicleDataAsset> VehicleData;
	
public:
	/** Constructor*/
	AAA_WheeledVehiclePawn(const class FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void Destroyed() override;

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

	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void CameraLookStart();

	UFUNCTION(BlueprintImplementableEvent,BlueprintCallable)
	void CameraLookFinish();


	///Cosmetic Functions
	UFUNCTION(BlueprintCallable, Category = VehicleVisuals)
	void SetVehiclePaint(int PaintIndex);
	UFUNCTION(BlueprintCallable, Category = VehicleVisuals)
	int GetPaintIndex();
	UFUNCTION(BlueprintCallable, Category = VehicleVisuals)
	int GetDecalIndex();
	UFUNCTION(BlueprintCallable, Category = VehicleVisuals)
	void SetVehicleDecal(int DecalIndex);
	UFUNCTION(BlueprintCallable, Category = VehicleVisuals)
	void SetColorOne(FColor ColorToSet);
	UFUNCTION(BlueprintCallable, BlueprintPure,Category = VehicleVisuals)
	FColor GetColorOne() const;
	UFUNCTION(BlueprintCallable, Category = VehicleVisuals)
	void SetColorTwo(FColor ColorToSet);
	UFUNCTION(BlueprintCallable, BlueprintPure,Category = VehicleVisuals)
	FColor GetColorTwo() const;
	UFUNCTION(BlueprintCallable, Category = VehicleVisuals)
	void SetColorThree(FColor ColorToSet);
	UFUNCTION(BlueprintCallable, BlueprintPure,Category = VehicleVisuals)
	FColor GetColorThree() const;
	UFUNCTION(BlueprintCallable, Category = VehicleVisuals)
	void SetColorFour(FColor ColorToSet);
	UFUNCTION(BlueprintCallable, BlueprintPure,Category = VehicleVisuals)
	FColor GetColorFour() const;
	UFUNCTION(BlueprintCallable, BlueprintPure,Category = VehicleVisuals)
	FColor GetColor(int Index) const;

	//Set Vehicle Properties
	UFUNCTION(BlueprintCallable, Category = VehicleControlSettings)
	void SetABSState(bool Enabled);

	UFUNCTION(BlueprintCallable, Category = VehicleControlSettings)
	void SetTractionControlState(bool Enabled);

	UFUNCTION(BlueprintCallable, Category = VehicleControlSettings)
	void BoostBrakingForce(float BrakeForceMultiplier);

	UFUNCTION(BlueprintCallable, Category = VehicleControlSettings)
	void SetAutomaticShifting(bool Enabled);

private:
	
	UPROPERTY()
	bool ABSEnabled = true;
	
	UPROPERTY()
	bool TractionControlEnabled = true;
	
	UPROPERTY()
	bool AutomaticShiftingEnabled = true;
	
public:
	/*
	* Sets realism for how much the current normal force on the car wheels affects steering and braking. 
	* If the value is 0 then the current normal force does not affect handling, and if it is 1 it takes full effect.
	*/
	UFUNCTION(BlueprintCallable, Category = VehicleControlSettings)
	void SetWheelLoadRatio(float WheelLoadRatio);

	//Pure Getters
	UFUNCTION(BlueprintPure, Category = Movement)
	float GetVehicleSpeed() const;

	UFUNCTION(BlueprintPure, Category = Movement)
	float GetVehicleSpeedMph() const;

	UFUNCTION(BlueprintPure)
	float GetVehicleLength() const;

	UFUNCTION(BlueprintPure)
	float GetVehicleHeight() const;

	UFUNCTION(BlueprintPure)
	float GetVehicleWidth() const;

	UFUNCTION(BlueprintPure)
	virtual FVector GetFrontWorldLocation() const;

	UFUNCTION(BlueprintPure)
	virtual FVector GetBackWorldLocation() const;

	UFUNCTION(BlueprintPure)
	virtual FVector GetTopWorldLocation() const;

	UFUNCTION(BlueprintPure)
	virtual FVector GetBottomWorldLocation() const;

	UFUNCTION(BlueprintPure)
	FBox GetAABB() const;

	/*
	* Gets a value [0,1] indicating how well the car is steering toward its intended target with 1 being perfectly on target.
	*/
	UFUNCTION(BlueprintPure)
	float GetTraction() const;

	UFUNCTION(BlueprintPure)
	bool IsAccelerating() const;

	UFUNCTION(BlueprintPure)
	bool IsBraking() const;

	UFUNCTION(BlueprintPure)
	bool IsReversing() const;

// Used for visual logger debug tool to display attributes about the actor - only enabled in non-shipping builds
#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif
	
	UPROPERTY()
	UMaterialInstanceDynamic* PaintMaterial;

	UPROPERTY(EditInstanceOnly, Category = VehicleVisuals)
	int PaintTextureIndex = 0;
	UPROPERTY(EditInstanceOnly, Category = VehicleVisuals)
	int DecalTextureIndex = 0;
	UPROPERTY(EditInstanceOnly, Category = VehicleVisuals)
	FColor ColorOne = FColor::Blue;
	UPROPERTY(EditInstanceOnly, Category = VehicleVisuals)
	FColor ColorTwo = FColor::White;
	UPROPERTY(EditInstanceOnly, Category = VehicleVisuals)
	FColor ColorThree = FColor::Black;
	UPROPERTY(EditInstanceOnly, Category = VehicleVisuals)
	FColor ColorFour = FColor::Green;
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty( FPropertyChangedEvent & PropertyChangedEvent ) override;
#endif

	//~ Begin AActor Interface
	virtual void DisplayDebug(class UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;

	UFUNCTION(BlueprintCallable)
	void SetVehicleData(UAA_VehicleDataAsset* NewVehicleData);
	
	/** Rewind Functions **/
	virtual void SetRewindTime(float Time) override;
	virtual void PauseRecordingSnapshots() override;
	virtual void ResumeRecordingSnapshots() override;
	virtual void ResetRewindHistory() override;
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

	bool SnapshotsPaused = false;
};

#pragma region Inline Definitions

inline FVector AAA_WheeledVehiclePawn::GetFrontWorldLocation() const
{
	return GetActorLocation() + GetActorForwardVector() * GetVehicleLength() * 0.5f;
}

inline FVector AAA_WheeledVehiclePawn::GetBackWorldLocation() const
{
	return GetActorLocation() - GetActorForwardVector() * GetVehicleLength() * 0.5f;
}

inline FVector AAA_WheeledVehiclePawn::GetBottomWorldLocation() const
{
	return GetActorLocation();
}

inline FVector AAA_WheeledVehiclePawn::GetTopWorldLocation() const
{
	return GetBottomWorldLocation() + FVector(0, 0, GetVehicleHeight());
}

inline float AAA_WheeledVehiclePawn::GetVehicleLength() const
{
	const FVector& VehicleExtent = GetAABB().GetExtent();
	return FMath::Max(VehicleExtent.X, VehicleExtent.Y) * 2;
}

inline float AAA_WheeledVehiclePawn::GetVehicleWidth() const
{
	const FVector& VehicleExtent = GetAABB().GetExtent();
	return FMath::Min(VehicleExtent.X, VehicleExtent.Y) * 2;
}

inline float AAA_WheeledVehiclePawn::GetVehicleHeight() const
{
	return GetAABB().GetExtent().Z * 2;
}

#pragma endregion Inline Definitions