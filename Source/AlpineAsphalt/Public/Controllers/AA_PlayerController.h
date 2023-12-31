// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "GameFramework/PlayerController.h"
#include "Interface/AA_InteractableInterface.h"
#include "VisualLogger/VisualLoggerDebugSnapshotInterface.h"

#include "Components/AA_PlayerRaceSplineInfoComponent.h"

#include "AA_PlayerController.generated.h"

class UAA_BaseUI;
class UAA_VehicleUI;
class AAA_WheeledVehiclePawn;
class AAA_TrackInfoActor;

DECLARE_LOG_CATEGORY_EXTERN(PlayerControllerLog, Log, All);
/**
 * 
 */
UCLASS()
class ALPINEASPHALT_API AAA_PlayerController : public APlayerController, public IVisualLoggerDebugSnapshotInterface
{
	GENERATED_BODY()

public:

	AAA_PlayerController();

	void SetTrackInfo(AAA_TrackInfoActor* TrackInfoActor);

	std::optional<FPlayerSplineInfo> GetPlayerSplineInfo() const;

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

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

	UPROPERTY(BlueprintReadOnly)
	UAA_VehicleUI* VehicleUI;

	UPROPERTY(BlueprintReadOnly)
	UAA_BaseUI* BaseUI;

	UPROPERTY(EditDefaultsOnly, Category = UI)
	TSubclassOf<UAA_VehicleUI> VehicleUIClass;

	UPROPERTY(EditDefaultsOnly, Category = UI)
	TSubclassOf<UAA_BaseUI> BaseUIClass;

	UPROPERTY(Category = "Activity", VisibleDefaultsOnly)
	TObjectPtr<UAA_PlayerRaceSplineInfoComponent> PlayerRaceSplineInfoComponent{};

	UFUNCTION()
	void OnRacerSettingsUpdated();
	
	virtual void OnPossess(APawn* InPawn) override;

	virtual void SetupInputComponent() override;

	void AddInteractables(IAA_InteractableInterface* Interactable);
	void RemoveInteractable(IAA_InteractableInterface* Interactable);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void OnUnPossess() override;

private:
	UPROPERTY(Transient)
	UEnhancedInputComponent* EInputComponent;

	UPROPERTY(Transient)
	AAA_WheeledVehiclePawn* VehiclePawn;

	UPROPERTY(Transient)
	TArray<IAA_InteractableInterface*> Interactables;

	/** Driver Control Functions**/
	void SetBrake(const FInputActionValue& Value);
	void SetThrottle(const FInputActionValue& Value);
	void SetHandbrake(const FInputActionValue& Value);
	void SetSteering(const FInputActionValue& Value);
	void CameraLookStart(const FInputActionValue& Value);
	void CameraLook(const FInputActionValue& Value);
	void CameraLookFinish(const FInputActionValue& Value);
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

	void InitDebugDraw();
	void DestroyDebugDraw();

	private:
	#if ENABLE_VISUAL_LOG
		FTimerHandle VisualLoggerTimer{};
	#endif
	
};
