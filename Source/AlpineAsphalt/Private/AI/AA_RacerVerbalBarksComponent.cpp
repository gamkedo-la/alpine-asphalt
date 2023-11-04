// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AA_RacerVerbalBarksComponent.h"

#include "VisualLogger/VisualLogger.h"
#include "Logging/LoggingUtils.h"
#include "Logging/AlpineAsphaltLogger.h"

#include "Controllers/AA_PlayerController.h"
#include "Kismet/GameplayStatics.h"

using namespace AA;

// Sets default values for this component's properties
UAA_RacerVerbalBarksComponent::UAA_RacerVerbalBarksComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = 0.5f;
}

void UAA_RacerVerbalBarksComponent::OnStuck(AAA_WheeledVehiclePawn* VehiclePawn, const FVector& IdealSeekPosition)
{
	// TODO:
}

// Called when the game starts
void UAA_RacerVerbalBarksComponent::BeginPlay()
{
	Super::BeginPlay();
	
	RacerContextProvider = Cast<IAA_RacerContextProvider>(GetOwner());

	if (!RacerContextProvider)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Error,
			TEXT("%s-%s: BeginPlay - Owner does not implement IAA_RacerContextProvider"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));

		SetComponentTickEnabled(false);
		return;
	}

	// TODO: Register hit events on vehicle mesh for detecting if we hit props or if player hits us
}

void UAA_RacerVerbalBarksComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CheckRelativePlayerPosition();
}

void UAA_RacerVerbalBarksComponent::CheckRelativePlayerPosition()
{
	check(RacerContextProvider);
	
	const auto& Context = RacerContextProvider->GetRacerContext();
	
	if (!PlayerController)
	{
		PlayerController = Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	}

	if (!PlayerController)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Warning,
			TEXT("%s-%s: CheckRelativePlayerPosition - PlayerController %s is not a AAA_PlayerController"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(UGameplayStatics::GetPlayerController(GetWorld(), 0)));

		return;
	}

	const auto& PlayerSplineInfo = PlayerController->GetPlayerSplineInfo();
	if (!PlayerSplineInfo)
	{
		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Warning,
			TEXT("%s-%s: CheckRelativePlayerPosition - PlayerController %s does not yet have spline info set"),
			*GetName(), *LoggingUtils::GetName(GetOwner()), *LoggingUtils::GetName(PlayerController));

		return;
	}

	const auto DistanceDelta = Context.CurrentDistanceAlongSpline - PlayerSplineInfo->DistanceAlongSpline;
	const bool bAheadOfPlayer = DistanceDelta >= 0;

	if (PlayerPositionChanges.IsEmpty())
	{
		PlayerPositionChanges.Add(bAheadOfPlayer);
		return;
	}

	if (bAheadOfPlayer && !PlayerPositionChanges.Last())
	{
		// TODO: Trigger audio for AI passing player at some given probability
		PlayerPositionChanges.Add(true);

		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Display,
			TEXT("%s-%s: CheckRelativePlayerPosition - AI Passed player"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
	}
	else if (!bAheadOfPlayer && PlayerPositionChanges.Last())
	{
		// TODO: Trigger audio for player passing AI at some given probability
		// TODO: Also note if player previous passed and now we are passing again to give player a jab 
		PlayerPositionChanges.Add(false);

		UE_VLOG_UELOG(GetOwner(), LogAlpineAsphalt, Display,
			TEXT("%s-%s: CheckRelativePlayerPosition - Player passed AI"),
			*GetName(), *LoggingUtils::GetName(GetOwner()));
	}
}

