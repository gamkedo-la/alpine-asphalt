// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MapCaptureComponent.h"

UMapCaptureComponent::UMapCaptureComponent()
{
	bWantsInitializeComponent = true;

	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// Default settings for maps
	SetRelativeRotation(FRotator{ -90, 0, 0 });

	ProjectionType = ECameraProjectionMode::Orthographic;
	// default to size of small test map
	// TODO: Will need to update dynamically from actor parent based on actual map loaded and possibly account for tiles with a big world partition map
	OrthoWidth = 25200;

	// Control capture when it is active, i.e. when map is visible
	bCaptureEveryFrame = false;
}

void UMapCaptureComponent::SetCaptureEnabled(bool bEnabled)
{
	bCaptureEveryFrame = bEnabled;
	SetComponentTickEnabled(bEnabled);
}

void UMapCaptureComponent::InitializeComponent()
{
	Super::InitializeComponent();
}
