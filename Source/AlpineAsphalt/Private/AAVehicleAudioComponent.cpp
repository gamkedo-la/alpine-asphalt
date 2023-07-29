// Fill out your copyright notice in the Description page of Project Settings.


#include "AAVehicleAudioComponent.h"
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
		EngineAudioComponent -> Sound = EngineSound;
	}
	else
	{
		// default to MSEngine Sound
		static ConstructorHelpers::FObjectFinder<USoundBase> DefaultEngineSoundFile(TEXT("/AlpineAsphalt/Content/Audio/Vehicles/MS_Engine"));
		EngineAudioComponent -> Sound = DefaultEngineSoundFile.Object;
	}
	// ...
}


// Called when the game starts
void UAAVehicleAudioComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UAAVehicleAudioComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

