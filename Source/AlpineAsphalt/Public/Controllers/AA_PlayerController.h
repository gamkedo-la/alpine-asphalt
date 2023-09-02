// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "GameFramework/PlayerController.h"
#include "Interface/AA_InteractableInterface.h"
#include "AA_PlayerController.generated.h"

class AAA_WheeledVehiclePawn;

DECLARE_LOG_CATEGORY_EXTERN(PlayerControllerLog, Log, All);
/**
 * 
 */
UCLASS()
class ALPINEASPHALT_API AAA_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:

	/** Driving Controls **/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	class UInputMappingContext* DefaultVehicleInputMapping;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputInteract;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputEnterRewind;

	/** Rewind Controls **/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputMappingContext* RewindInputMapping;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputRewindTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputFastForwardTime;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputConfirmRewind;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	UInputAction* InputCancelRewind;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float RewindSpeed = .1f;
	
	virtual void OnPossess(APawn* InPawn) override;

	virtual void SetupInputComponent() override;

	void AddInteractables(IAA_InteractableInterface* Interactable);
	void RemoveInteractable(IAA_InteractableInterface* Interactable);

private:
	UPROPERTY()
	UEnhancedInputComponent* EInputComponent;

	UPROPERTY()
	AAA_WheeledVehiclePawn* VehiclePawn;

	UPROPERTY()
	TArray<IAA_InteractableInterface*> Interactables;

	/** Driver Control Functions**/
	void SetBrake(const FInputActionValue& Value);
	void SetThrottle(const FInputActionValue& Value);
	void SetHandbrake(const FInputActionValue& Value);
	void SetSteering(const FInputActionValue& Value);
	void CameraLook(const FInputActionValue& Value);
	void ResetVehicle(const FInputActionValue& Value);
	void ShiftUp(const FInputActionValue& Value);
	void ShiftDown(const FInputActionValue& Value);
	void ToggleCamera(const FInputActionValue& Value);
	void Interact(const FInputActionValue& Value);
	void EnterRewindMode(const FInputActionValue& Value);

	/** Rewind Mode Controls**/
	void ConfirmRewind(const FInputActionValue& Value);
	void CancelRewind(const FInputActionValue& Value);
	void RewindTime(const FInputActionValue& Value);
	void FastForwardTime(const FInputActionValue& Value);
	
};
