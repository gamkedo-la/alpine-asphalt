// Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/AA_PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Blueprint/UserWidget.h"
#include "Pawn/AA_WheeledVehiclePawn.h"
#include "Subsystems/AA_RewindSubsystem.h"
#include "UI/AA_VehicleUI.h"
#include "UI/AA_BaseUI.h"

#include "Logging/AlpineAsphaltLogger.h"
#include "UI/AA_GameUserSettings.h"
#include "VisualLogger/VisualLogger.h"

DEFINE_LOG_CATEGORY(PlayerControllerLog);

void AAA_PlayerController::OnRacerSettingsUpdated()
{
	if (const auto GameUserSettings = UAA_GameUserSettings::GetInstance())
	{
		VehiclePawn->SetAutomaticShifting(GameUserSettings->GetAutomaticShifting());
		VehiclePawn->SetABSState(GameUserSettings->GetABSBrakesEnabled());
		VehiclePawn->SetTractionControlState(GameUserSettings->GetTractionControlEnabled());

		VehicleUI->SetSpeedUnitKPH(GameUserSettings->GetSpeedUnitKPH());

		//TODO probably bind this in RewindSubsystem itself?
		GetWorld()->GetSubsystem<UAA_RewindSubsystem>()->EnableRewindMode(GameUserSettings->GetRewindTimeEnabled());
	}
	else
	{
		UE_VLOG_UELOG(this, LogAlpineAsphalt, Error, TEXT("%s: %hs: GameUserSettings is NULL - using a default value"),
		              *GetName(),__FUNCTION__);
	}
}

void AAA_PlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	VehiclePawn = Cast<AAA_WheeledVehiclePawn>(InPawn);

	if(!VehiclePawn)
	{
		UE_LOG(PlayerControllerLog,Error,TEXT("%hs: Pawn was not an AA_WheeledVehiclePawn_CPP"),__FUNCSIG__);
	}

	BaseUI = CreateWidget<UAA_BaseUI>(GetGameInstance(),BaseUIClass);
	BaseUI->AddToViewport(0);

	VehicleUI = Cast<UAA_VehicleUI>(BaseUI->PushHUD(VehicleUIClass));

	OnRacerSettingsUpdated();

	const auto GameUserSettings = UAA_GameUserSettings::GetInstance();
	GameUserSettings->OnGameUserSettingsUpdated.AddDynamic(this, &ThisClass::OnRacerSettingsUpdated);
}

void AAA_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	Subsystem->ClearAllMappings();
	Subsystem->AddMappingContext(DefaultVehicleInputMapping, 0);
	EInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	
	//Brakes
	EInputComponent->BindAction(InputBrake,ETriggerEvent::Triggered, this, &AAA_PlayerController::SetBrake);
	EInputComponent->BindAction(InputBrake,ETriggerEvent::Ongoing, this, &AAA_PlayerController::SetBrake);
	EInputComponent->BindAction(InputBrake,ETriggerEvent::Completed, this, &AAA_PlayerController::SetBrake);

	//Throttle
	EInputComponent->BindAction(InputThrottle,ETriggerEvent::Triggered, this, &AAA_PlayerController::SetThrottle);
	EInputComponent->BindAction(InputThrottle,ETriggerEvent::Ongoing, this, &AAA_PlayerController::SetThrottle);
	EInputComponent->BindAction(InputThrottle,ETriggerEvent::Completed, this, &AAA_PlayerController::SetThrottle);

	//Handbrake
	EInputComponent->BindAction(InputHandbrake,ETriggerEvent::Triggered, this, &AAA_PlayerController::SetHandbrake);
	EInputComponent->BindAction(InputHandbrake,ETriggerEvent::Completed, this, &AAA_PlayerController::SetHandbrake);

	//Steering
	EInputComponent->BindAction(InputSteering,ETriggerEvent::Triggered, this, &AAA_PlayerController::SetSteering);
	EInputComponent->BindAction(InputSteering,ETriggerEvent::Ongoing, this, &AAA_PlayerController::SetSteering);
	EInputComponent->BindAction(InputSteering,ETriggerEvent::Completed, this, &AAA_PlayerController::SetSteering);

	//Camera Look
	EInputComponent->BindAction(InputLookAround,ETriggerEvent::Started, this, &AAA_PlayerController::CameraLookStart);
	EInputComponent->BindAction(InputLookAround,ETriggerEvent::Triggered, this, &AAA_PlayerController::CameraLook);
	EInputComponent->BindAction(InputLookAround,ETriggerEvent::Completed, this, &AAA_PlayerController::CameraLookFinish);
	
	//Reset Vehicle
	EInputComponent->BindAction(InputReset,ETriggerEvent::Started, this, &AAA_PlayerController::ResetVehicle);

	//Shift Up
	EInputComponent->BindAction(InputShiftUp,ETriggerEvent::Started, this, &AAA_PlayerController::ShiftUp);
	
	//Shift Down
	EInputComponent->BindAction(InputShiftDown,ETriggerEvent::Started, this, &AAA_PlayerController::ShiftDown);

	//Toggle Camera
	EInputComponent->BindAction(InputToggleCamera,ETriggerEvent::Started, this, &AAA_PlayerController::ToggleCamera);

	//Interact
	EInputComponent->BindAction(InputInteract,ETriggerEvent::Started, this, &AAA_PlayerController::Interact);


	//Enter Rewind Mode
	EInputComponent->BindAction(InputEnterRewind,ETriggerEvent::Triggered, this, &AAA_PlayerController::EnterRewindMode);

	//Confirm Rewind
	EInputComponent->BindAction(InputConfirmRewind,ETriggerEvent::Triggered, this, &AAA_PlayerController::ConfirmRewind);

	//Cancel Rewind
	EInputComponent->BindAction(InputCancelRewind,ETriggerEvent::Triggered, this, &AAA_PlayerController::CancelRewind);

	//Rewind Time
	EInputComponent->BindAction(InputRewindTime,ETriggerEvent::Triggered, this, &AAA_PlayerController::RewindTime);
	EInputComponent->BindAction(InputRewindTime,ETriggerEvent::Ongoing, this, &AAA_PlayerController::RewindTime);
	EInputComponent->BindAction(InputRewindTime,ETriggerEvent::Completed, this, &AAA_PlayerController::RewindTime);

	//Fast Forward Time
	EInputComponent->BindAction(InputFastForwardTime,ETriggerEvent::Triggered, this, &AAA_PlayerController::FastForwardTime);
	EInputComponent->BindAction(InputFastForwardTime,ETriggerEvent::Ongoing, this, &AAA_PlayerController::FastForwardTime);
	EInputComponent->BindAction(InputFastForwardTime,ETriggerEvent::Completed, this, &AAA_PlayerController::FastForwardTime);

	
}

void AAA_PlayerController::AddInteractables(IAA_InteractableInterface* Interactable)
{
	Interactables.Add(Interactable);
}

void AAA_PlayerController::RemoveInteractable(IAA_InteractableInterface* Interactable)
{
	Interactables.Remove(Interactable);
}

void AAA_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	InitDebugDraw();
}

void AAA_PlayerController::SetBrake(const FInputActionValue& Value) 
{
	VehiclePawn->SetBrake(Value.Get<float>());
}

void AAA_PlayerController::SetThrottle(const FInputActionValue& Value) 
{
	VehiclePawn->SetThrottle(Value.Get<float>());
}

void AAA_PlayerController::SetHandbrake(const FInputActionValue& Value) 
{
	VehiclePawn->SetHandbrake(Value.Get<bool>());
}

void AAA_PlayerController::SetSteering(const FInputActionValue& Value) 
{
	VehiclePawn->SetSteering(Value.Get<float>());
}

void AAA_PlayerController::CameraLookStart(const FInputActionValue& Value)
{
	VehiclePawn->CameraLookStart();
}

void AAA_PlayerController::CameraLook(const FInputActionValue& Value) 
{
	VehiclePawn->CameraLook(Value.Get<FVector2D>());
}

void AAA_PlayerController::CameraLookFinish(const FInputActionValue& Value)
{
	VehiclePawn->CameraLookFinish();
}

void AAA_PlayerController::ResetVehicle(const FInputActionValue& Value) 
{
	VehiclePawn->ResetVehicle();
}

void AAA_PlayerController::ShiftUp(const FInputActionValue& Value) 
{
	VehiclePawn->ShiftUp();
}

void AAA_PlayerController::ShiftDown(const FInputActionValue& Value) 
{
	VehiclePawn->ShiftDown();
}

void AAA_PlayerController::ToggleCamera(const FInputActionValue& Value) 
{
	VehiclePawn->ToggleCamera();
}

void AAA_PlayerController::Interact(const FInputActionValue& Value)
{
	//Just interact with the first thing in the set
	if(Interactables.Num() > 0)
	{
		Interactables[0]->Interact(this);
	}
}

void AAA_PlayerController::EnterRewindMode(const FInputActionValue& Value)
{
	UE_LOG(PlayerControllerLog,Log,TEXT("EnterRewindMode Pressed"));
	
	if(UAA_RewindSubsystem* RewindSystem = GetWorld()->GetSubsystem<UAA_RewindSubsystem>())
	{
		//set active
		if(RewindSystem->EnterRewindMode())
		{
			//Setup Input
			UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(RewindInputMapping, 0);
		}
	}else
	{
		UE_LOG(PlayerControllerLog,Error,TEXT("Rewind Subsystem Unavailable"))
	}
}

void AAA_PlayerController::ConfirmRewind(const FInputActionValue& Value)
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

void AAA_PlayerController::CancelRewind(const FInputActionValue& Value)
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

void AAA_PlayerController::RewindTime(const FInputActionValue& Value)
{
	if(UAA_RewindSubsystem* RewindSystem= GetWorld()->GetSubsystem<UAA_RewindSubsystem>())
	{
		RewindSystem->Rewind(Value.Get<float>()*RewindSpeed);
	}else
	{
		UE_LOG(PlayerControllerLog,Error,TEXT("Rewind Subsystem Unavailable"))
	}
}

void AAA_PlayerController::FastForwardTime(const FInputActionValue& Value)
{
	if(UAA_RewindSubsystem* RewindSystem= GetWorld()->GetSubsystem<UAA_RewindSubsystem>())
	{
		RewindSystem->FastForward(Value.Get<float>()*RewindSpeed);
	}else
	{
		UE_LOG(PlayerControllerLog,Error,TEXT("Rewind Subsystem Unavailable"))
	}
}

void AAA_PlayerController::InitDebugDraw()
{

// Ensure that vehicle state logged regularly so we see the updates in the visual logger
#if ENABLE_VISUAL_LOG

	FTimerDelegate DebugDrawDelegate = FTimerDelegate::CreateLambda([this]()
	{
		UE_VLOG(this, LogAlpineAsphalt, Log, TEXT("Get Vehicle State"));
	});

	GetWorldTimerManager().SetTimer(VisualLoggerTimer, DebugDrawDelegate, 0.05f, true);

#endif
}

void AAA_PlayerController::DestroyDebugDraw()
{
#if ENABLE_VISUAL_LOG

	GetWorldTimerManager().ClearTimer(VisualLoggerTimer);

#endif
}
