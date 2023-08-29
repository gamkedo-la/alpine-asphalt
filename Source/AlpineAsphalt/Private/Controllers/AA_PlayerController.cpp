// Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/AA_PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Pawn/AA_WheeledVehiclePawn.h"
#include "Subsystems/AA_RewindSubsystem.h"

DEFINE_LOG_CATEGORY(PlayerControllerLog);

void AAA_PlayerController_CPP::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	VehiclePawn = Cast<AAA_WheeledVehiclePawn>(InPawn);

	if(!VehiclePawn)
	{
		UE_LOG(PlayerControllerLog,Error,TEXT("%hs: Pawn was not an AA_WheeledVehiclePawn_CPP"),__FUNCSIG__);
	}
}

void AAA_PlayerController_CPP::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	Subsystem->ClearAllMappings();
	Subsystem->AddMappingContext(DefaultVehicleInputMapping, 0);
	EInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	
	//Brakes
	EInputComponent->BindAction(InputBrake,ETriggerEvent::Triggered, this, &AAA_PlayerController_CPP::SetBrake);
	EInputComponent->BindAction(InputBrake,ETriggerEvent::Ongoing, this, &AAA_PlayerController_CPP::SetBrake);
	EInputComponent->BindAction(InputBrake,ETriggerEvent::Completed, this, &AAA_PlayerController_CPP::SetBrake);

	//Throttle
	EInputComponent->BindAction(InputThrottle,ETriggerEvent::Triggered, this, &AAA_PlayerController_CPP::SetThrottle);
	EInputComponent->BindAction(InputThrottle,ETriggerEvent::Ongoing, this, &AAA_PlayerController_CPP::SetThrottle);
	EInputComponent->BindAction(InputThrottle,ETriggerEvent::Completed, this, &AAA_PlayerController_CPP::SetThrottle);

	//Handbrake
	EInputComponent->BindAction(InputHandbrake,ETriggerEvent::Triggered, this, &AAA_PlayerController_CPP::SetHandbrake);
	EInputComponent->BindAction(InputHandbrake,ETriggerEvent::Completed, this, &AAA_PlayerController_CPP::SetHandbrake);

	//Steering
	EInputComponent->BindAction(InputSteering,ETriggerEvent::Triggered, this, &AAA_PlayerController_CPP::SetSteering);
	EInputComponent->BindAction(InputSteering,ETriggerEvent::Ongoing, this, &AAA_PlayerController_CPP::SetSteering);
	EInputComponent->BindAction(InputSteering,ETriggerEvent::Completed, this, &AAA_PlayerController_CPP::SetSteering);

	//Camera Look
	EInputComponent->BindAction(InputLookAround,ETriggerEvent::Triggered, this, &AAA_PlayerController_CPP::CameraLook);
	EInputComponent->BindAction(InputLookAround,ETriggerEvent::Ongoing, this, &AAA_PlayerController_CPP::CameraLook);
	EInputComponent->BindAction(InputLookAround,ETriggerEvent::Completed, this, &AAA_PlayerController_CPP::CameraLook);
	
	//Reset Vehicle
	EInputComponent->BindAction(InputReset,ETriggerEvent::Triggered, this, &AAA_PlayerController_CPP::ResetVehicle);

	//Shift Up
	EInputComponent->BindAction(InputShiftUp,ETriggerEvent::Triggered, this, &AAA_PlayerController_CPP::ShiftUp);
	
	//Shift Down
	EInputComponent->BindAction(InputShiftDown,ETriggerEvent::Triggered, this, &AAA_PlayerController_CPP::ShiftDown);

	//Toggle Camera
	EInputComponent->BindAction(InputToggleCamera,ETriggerEvent::Triggered, this, &AAA_PlayerController_CPP::ToggleCamera);

	//Enter Rewind Mode
	EInputComponent->BindAction(InputEnterRewind,ETriggerEvent::Triggered, this, &AAA_PlayerController_CPP::EnterRewindMode);

	//Confirm Rewind
	EInputComponent->BindAction(InputConfirmRewind,ETriggerEvent::Triggered, this, &AAA_PlayerController_CPP::ConfirmRewind);

	//Cancel Rewind
	EInputComponent->BindAction(InputCancelRewind,ETriggerEvent::Triggered, this, &AAA_PlayerController_CPP::CancelRewind);

	//Rewind Time
	EInputComponent->BindAction(InputRewindTime,ETriggerEvent::Triggered, this, &AAA_PlayerController_CPP::RewindTime);
	EInputComponent->BindAction(InputRewindTime,ETriggerEvent::Ongoing, this, &AAA_PlayerController_CPP::RewindTime);
	EInputComponent->BindAction(InputRewindTime,ETriggerEvent::Completed, this, &AAA_PlayerController_CPP::RewindTime);

	//Fast Forward Time
	EInputComponent->BindAction(InputFastForwardTime,ETriggerEvent::Triggered, this, &AAA_PlayerController_CPP::FastForwardTime);
	EInputComponent->BindAction(InputFastForwardTime,ETriggerEvent::Ongoing, this, &AAA_PlayerController_CPP::FastForwardTime);
	EInputComponent->BindAction(InputFastForwardTime,ETriggerEvent::Completed, this, &AAA_PlayerController_CPP::FastForwardTime);

	
}

void AAA_PlayerController_CPP::SetBrake(const FInputActionValue& Value) 
{
	VehiclePawn->SetBrake(Value.Get<float>());
}

void AAA_PlayerController_CPP::SetThrottle(const FInputActionValue& Value) 
{
	VehiclePawn->SetThrottle(Value.Get<float>());
}

void AAA_PlayerController_CPP::SetHandbrake(const FInputActionValue& Value) 
{
	VehiclePawn->SetHandbrake(Value.Get<bool>());
}

void AAA_PlayerController_CPP::SetSteering(const FInputActionValue& Value) 
{
	VehiclePawn->SetSteering(Value.Get<float>());
}

void AAA_PlayerController_CPP::CameraLook(const FInputActionValue& Value) 
{
	VehiclePawn->CameraLook(Value.Get<FVector2D>());
}

void AAA_PlayerController_CPP::ResetVehicle(const FInputActionValue& Value) 
{
	VehiclePawn->ResetVehicle();
}

void AAA_PlayerController_CPP::ShiftUp(const FInputActionValue& Value) 
{
	VehiclePawn->ShiftUp();
}

void AAA_PlayerController_CPP::ShiftDown(const FInputActionValue& Value) 
{
	VehiclePawn->ShiftDown();
}

void AAA_PlayerController_CPP::ToggleCamera(const FInputActionValue& Value) 
{
	VehiclePawn->ToggleCamera();
}

void AAA_PlayerController_CPP::EnterRewindMode(const FInputActionValue& Value)
{
	UE_LOG(PlayerControllerLog,Log,TEXT("EnterRewindMode Pressed"));
	
	if(UAA_RewindSubsystem* RewindSystem = GetWorld()->GetSubsystem<UAA_RewindSubsystem>())
	{
		//don't do anything if already active
		if(RewindSystem->IsRewindModeActive()){return;}

		//set active
		RewindSystem->EnterRewindMode();
		//Setup Input
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
		Subsystem->ClearAllMappings();
		Subsystem->AddMappingContext(RewindInputMapping, 0);
	}else
	{
		UE_LOG(PlayerControllerLog,Error,TEXT("Rewind Subsystem Unavailable"))
	}
}

void AAA_PlayerController_CPP::ConfirmRewind(const FInputActionValue& Value)
{
	UE_LOG(PlayerControllerLog,Log,TEXT("ConfirmRewind Pressed"));

	if(UAA_RewindSubsystem* RewindSystem= GetWorld()->GetSubsystem<UAA_RewindSubsystem>())
	{
		RewindSystem->ConfirmRewind();
	}else
	{
		UE_LOG(PlayerControllerLog,Error,TEXT("Rewind Subsystem Unavailable"))
	}

	//Setup Input
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	Subsystem->ClearAllMappings();
	Subsystem->AddMappingContext(DefaultVehicleInputMapping, 0);
}

void AAA_PlayerController_CPP::CancelRewind(const FInputActionValue& Value)
{
	UE_LOG(PlayerControllerLog,Verbose,TEXT("CancelRewind Pressed"));
	
	if(UAA_RewindSubsystem* RewindSystem= GetWorld()->GetSubsystem<UAA_RewindSubsystem>())
	{
		RewindSystem->CancelRewindMode();
	}else
	{
		UE_LOG(PlayerControllerLog,Error,TEXT("Rewind Subsystem Unavailable"))
	}

	//Setup Input
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	Subsystem->ClearAllMappings();
	Subsystem->AddMappingContext(DefaultVehicleInputMapping, 0);
}

void AAA_PlayerController_CPP::RewindTime(const FInputActionValue& Value)
{
	if(UAA_RewindSubsystem* RewindSystem= GetWorld()->GetSubsystem<UAA_RewindSubsystem>())
	{
		RewindSystem->Rewind(Value.Get<float>()*RewindSpeed);
	}else
	{
		UE_LOG(PlayerControllerLog,Error,TEXT("Rewind Subsystem Unavailable"))
	}
}

void AAA_PlayerController_CPP::FastForwardTime(const FInputActionValue& Value)
{
	if(UAA_RewindSubsystem* RewindSystem= GetWorld()->GetSubsystem<UAA_RewindSubsystem>())
	{
		RewindSystem->FastForward(Value.Get<float>()*RewindSpeed);
	}else
	{
		UE_LOG(PlayerControllerLog,Error,TEXT("Rewind Subsystem Unavailable"))
	}
}
