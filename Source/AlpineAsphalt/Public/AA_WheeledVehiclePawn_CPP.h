// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AA_WheeledVehiclePawn_CPP.generated.h"

//Define Log for Vehicle
DECLARE_LOG_CATEGORY_EXTERN(Vehicle, Log, All);
/**
 * 
 */
UCLASS()
class ALPINEASPHALT_API AAA_WheeledVehiclePawn_CPP : public APawn
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

	/** Name of the MeshComponent. Use this name if you want to prevent creation of the component (with ObjectInitializer.DoNotCreateDefaultSubobject). */
	static FName VehicleMeshComponentName;

	/** Name of the VehicleMovement. Use this name if you want to use a different class (with ObjectInitializer.SetDefaultSubobjectClass). */
	static FName VehicleMovementComponentName;

	/** Util to get the wheeled vehicle movement component */
	UChaosWheeledVehicleMovementComponent* GetVehicleMovementComponent() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty( FPropertyChangedEvent & PropertyChangedEvent ) override;
#endif

	//~ Begin AActor Interface
	virtual void DisplayDebug(class UCanvas* Canvas, const FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos) override;

	UFUNCTION(BlueprintCallable)
	void SetVehicleData(UAA_VehicleDataAsset_CPP* NewVehicleData);

};
