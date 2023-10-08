// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/AA_VehicleAudioComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/AudioComponent.h"
#include "Pawn/AA_WheeledVehiclePawn.h"

FName UAA_VehicleAudioComponent::VehicleAudioComponentName(TEXT("VehicleAudioComponentName"));
// Sets default values for this component's properties
UAA_VehicleAudioComponent::UAA_VehicleAudioComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	EngineAudioComponent = CreateDefaultSubobject<UAudioComponent>(VehicleAudioComponentName);
}


// Called when the game starts
void UAA_VehicleAudioComponent::BeginPlay()
{
	Super::BeginPlay();

	EngineAudioComponent->SetActive(true);
	EngineAudioComponent->SetVolumeMultiplier(1.5);

	if(EngineAudioComponent != nullptr)
	{
		EngineAudioComponent->Play();
	}
}


// Called every frame
void UAA_VehicleAudioComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateEnginePitch();
	// ...
}

void UAA_VehicleAudioComponent::UpdateEnginePitch() const
{
	AAA_WheeledVehiclePawn* const parent = (AAA_WheeledVehiclePawn*) GetOwner();
	if (UChaosWheeledVehicleMovementComponent* VehicleMovementComponent = parent->GetVehicleMovementComponent())
	{
		float const EngineRotationSpeed = VehicleMovementComponent->GetEngineRotationSpeed();
		float EnginePitch = MinEnginePitch;
		if (EngineRotationSpeed > 2000.0f && EngineRotationSpeed <= 3000.0f)
		{
			EnginePitch = MidEnginePitch;
		}
		else if (EngineRotationSpeed > 3000.0f)
		{
			EnginePitch = MaxEnginePitch;	
		}
		EngineAudioComponent->SetPitchMultiplier(EnginePitch);
	}

}

