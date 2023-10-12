// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AA_AIRacerController.generated.h"

class UAA_AIVehicleControlComponent;

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

private:
	UPROPERTY(Category = "Movement", VisibleDefaultsOnly)
	TObjectPtr<UAA_AIVehicleControlComponent> VehicleControlComponent;
};
