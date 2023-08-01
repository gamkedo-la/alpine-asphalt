// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "GameFramework/PlayerController.h"
#include "AA_PlayerController_CPP.generated.h"

class AAA_WheeledVehiclePawn_CPP;
/**
 * 
 */
UCLASS()
class ALPINEASPHALT_API AAA_PlayerController_CPP : public APlayerController
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UInputMappingContext* InputMapping;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputBrake;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputHandbrake;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputLookAround;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputReset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputShiftDown;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputShiftUp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputSteering;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputThrottle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputToggleCamera;

	virtual void OnPossess(APawn* InPawn) override;

	virtual void SetupInputComponent() override;

private:
	UPROPERTY()
	UEnhancedInputComponent* EInputComponent;

	UPROPERTY()
	AAA_WheeledVehiclePawn_CPP* VehiclePawn;

	void SetBrake(const FInputActionValue& Value);
	void SetThrottle(const FInputActionValue& Value);
	void SetHandbrake(const FInputActionValue& Value);
	void SetSteering(const FInputActionValue& Value);
	void CameraLook(const FInputActionValue& Value);
	void ResetVehicle(const FInputActionValue& Value);
	void ShiftUp(const FInputActionValue& Value);
	void ShiftDown(const FInputActionValue& Value);
	void ToggleCamera(const FInputActionValue& Value);
	
};
