// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "AI/AA_AIRacerEvents.h"

#include "AA_RacerSplineFollowingComponent.generated.h"

class AAA_WheeledVehiclePawn;
class ALandscape;
class IAA_RacerContextProvider;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALPINEASPHALT_API UAA_RacerSplineFollowingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAA_RacerSplineFollowingComponent();

	UFUNCTION()
	void SelectNewMovementTarget(AAA_WheeledVehiclePawn* VehiclePawn, const FVector& PreviousMovementTarget);

	UFUNCTION()
	void OnVehicleAvoidancePositionUpdated(AAA_WheeledVehiclePawn* VehiclePawn, const FAA_AIRacerAvoidanceContext& AvoidanceContext);

#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisualLogEntry* Snapshot) const;
#endif // ENABLE_VISUAL_LOG

protected:
	virtual void BeginPlay() override;

private:

	ALandscape* GetLandscapeActor() const;
	FVector ClampTargetToGround(const FVector& Position) const;

	// TODO: May respond to a race start event
	void SetInitialMovementTarget();

public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	mutable FOnVehicleTargetUpdated OnVehicleTargetUpdated {};

private:
	IAA_RacerContextProvider* RacerContextProvider{};

	UPROPERTY(Category = "Movement", EditAnywhere)
	float LookaheadDistance{ 1000.0f };

	UPROPERTY(Transient)
	ALandscape* Landscape{};
		
};
