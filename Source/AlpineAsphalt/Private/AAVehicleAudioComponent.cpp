// Fill out your copyright notice in the Description page of Project Settings.


#include "AAVehicleAudioComponent.h"

#include "AA_WheeledVehiclePawn_CPP.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/AudioComponent.h"

FName UAAVehicleAudioComponent::VehicleAudioComponentName(TEXT("VehicleAudioComponentName"));
// Sets default values for this component's properties
UAAVehicleAudioComponent::UAAVehicleAudioComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	EngineAudioComponent = CreateDefaultSubobject<UAudioComponent>(VehicleAudioComponentName);
	if (EngineSound)
	{
		EngineAudioComponent -> SetSound(EngineSound);
	}
	else
	{
		// default to MSEngine Sound
		//TO:DO Currently not working, Need to read more documentation to determine why I am not setting the default
		// engine sound correctly
		static ConstructorHelpers::FObjectFinder<USoundBase> DefaultEngineSoundFile(TEXT("'/Game/Audio/Vehicles/MS_Engine'"));
		EngineAudioComponent -> SetSound(DefaultEngineSoundFile.Object);
	}
	EngineAudioComponent->SetActive(true);
	EngineAudioComponent->SetVolumeMultiplier(1.5);
	// ...
}


// Called when the game starts
void UAAVehicleAudioComponent::BeginPlay()
{
	Super::BeginPlay();
	if(EngineAudioComponent != nullptr)
	{
		EngineAudioComponent->Play();
	}

	// ...
	
}


// Called every frame
void UAAVehicleAudioComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateEnginePitch();
	// ...
}

void UAAVehicleAudioComponent::UpdateEnginePitch() const
{
	AAA_WheeledVehiclePawn_CPP* const parent = (AAA_WheeledVehiclePawn_CPP*) GetOwner();
	if (UChaosWheeledVehicleMovementComponent* VehicleMovementComponent = parent->GetVehicleMovementComponent())
	{
		float const EngineRotationSpeed = VehicleMovementComponent->GetEngineRotationSpeed();
		float EnginePitch = 0.5f;
		if (EngineRotationSpeed > 2000.0f && EngineRotationSpeed <= 3000.0f)
		{
			EnginePitch = 1.0f;
		}
		else if (EngineRotationSpeed > 3000.0f)
		{
			EnginePitch = 1.5f;	
		}
		EngineAudioComponent->SetPitchMultiplier(EnginePitch);
	}

}

