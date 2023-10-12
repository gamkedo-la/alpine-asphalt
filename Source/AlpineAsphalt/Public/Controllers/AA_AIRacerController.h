// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AA_AIRacerController.generated.h"

class UAA_AIVehicleControlComponent;
class AAA_WheeledVehiclePawn;
class ALandscape;

/**
 * 
 */
UCLASS()
class ALPINEASPHALT_API AAA_AIRacerController : public AAIController
{
	GENERATED_BODY()

public:
	AAA_AIRacerController();

	UFUNCTION(BlueprintCallable)
	UAA_AIVehicleControlComponent* GetVehicleControlComponent() const { return VehicleControlComponent; }

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

protected:

	virtual void BeginPlay() override;

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override final;

private:
	UFUNCTION()
	void SelectNewMovementTarget(AAA_WheeledVehiclePawn* VehiclePawn, const FVector& PreviousMovementTarget);

	ALandscape* GetLandscapeActor() const;
	FVector ClampTargetToGround(const FVector& Position) const;

private:
	UPROPERTY(Category = "Movement", VisibleDefaultsOnly)
	TObjectPtr<UAA_AIVehicleControlComponent> VehicleControlComponent;

	UPROPERTY(Category = "Movement", EditAnywhere)
	float LookaheadDistance{ 1000.0f };

	UPROPERTY(Transient)
	ALandscape* Landscape{};
};
