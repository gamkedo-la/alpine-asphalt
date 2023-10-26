// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/AA_MapCaptureActor.h"

#include "UI/MapCaptureComponent.h"
#include "Components/SphereComponent.h" 

AAA_MapCaptureActor::AAA_MapCaptureActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USphereComponent>(FName("Root"));

	MapCaptureComponent = CreateDefaultSubobject<UMapCaptureComponent>(TEXT("MapCapture"));
	MapCaptureComponent->SetupAttachment(RootComponent);
}

void AAA_MapCaptureActor::BeginPlay()
{
	Super::BeginPlay();
}
