// Fill out your copyright notice in the Description page of Project Settings.


#include "AA_PlayerController_CPP.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "AA_WheeledVehiclePawn_CPP.h"


void AAA_PlayerController_CPP::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	VehiclePawn = Cast<AAA_WheeledVehiclePawn_CPP>(InPawn);
	VehiclePawn->AutoReceiveInput = EAutoReceiveInput::Player0;
	VehiclePawn->AutoPossessPlayer = EAutoReceiveInput::Player0;
	UE_LOG(LogTemp, Error, TEXT("Possessed?"));
	if(!VehiclePawn)
	{
		UE_LOG(LogTemp,Error,TEXT("%hs: Pawn was not an AA_WheeledVehiclePawn_CPP"),__FUNCSIG__);
	}
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	Subsystem->ClearAllMappings();
	Subsystem->AddMappingContext(VehiclePawn->InputMapping, 0);
}

void AAA_PlayerController_CPP::SetupInputComponent()
{
	Super::SetupInputComponent();
	UE_LOG(LogTemp, Error, TEXT("Setup Input Comp..."));

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
	
}

void AAA_PlayerController_CPP::SetBrake(const FInputActionValue& Value) 
{
	VehiclePawn->SetBrake(Value.Get<float>());
}

void AAA_PlayerController_CPP::SetThrottle(const FInputActionValue& Value) 
{
	UE_LOG(LogTemp, Error, TEXT("Throttle called"));
	VehiclePawn->SetThrottle(Value.Get<float>());
}

void AAA_PlayerController_CPP::SetHandbrake(const FInputActionValue& Value) 
{
	VehiclePawn->SetHandbrake(Value.Get<bool>());
}

void AAA_PlayerController_CPP::SetSteering(const FInputActionValue& Value) 
{
	VehiclePawn->SetBrake(Value.Get<float>());
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