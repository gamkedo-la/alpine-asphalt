// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/AA_AIRacerEvents.h"

#include <optional>

#include "AA_RacerObstacleAvoidanceComponent.generated.h"

class IAA_RacerContextProvider;
class AAA_WheeledVehiclePawn;
struct FAA_AIRacerContext;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALPINEASPHALT_API UAA_RacerObstacleAvoidanceComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UAA_RacerObstacleAvoidanceComponent();

	UFUNCTION()
	void OnVehicleObstaclesUpdated(AAA_WheeledVehiclePawn* VehiclePawn, const TArray<AAA_WheeledVehiclePawn*>& VehicleObstacles);

#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisualLogEntry* Snapshot) const;
#endif // ENABLE_VISUAL_LOG

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	mutable FOnVehicleAvoidancePositionUpdated OnVehicleAvoidancePositionUpdated {};

private:
	struct FThreatContext;

	std::optional<FVector> ComputeThreatVector(const FThreatContext& ThreatContext, const AAA_WheeledVehiclePawn& CandidateVehicle) const;

	bool PopulateThreatContext(FThreatContext& ThreatContext) const;

private:
	IAA_RacerContextProvider* RacerContextProvider{};
};

