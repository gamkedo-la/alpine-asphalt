// Fill out your copyright notice in the Description page of Project Settings.


#include "AAVehicleAudioComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/AudioComponent.h"
#include "AA_WheeledVehiclePawn_CPP.h"

FName UAAVehicleAudioComponent::VehicleAudioComponentName(TEXT("VehicleAudioComponentName"));
// Sets default values for this component's properties
UAAVehicleAudioComponent::UAAVehicleAudioComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	EngineAudioComponent = CreateDefaultSubobject<UAudioComponent>(VehicleAudioComponentName);
	EngineAudioComponent->SetActive(true);
	EngineAudioComponent->SetVolumeMultiplier(1.5);
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

