// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/AA_AIRacerEvents.h"
#include "Interface/AA_RecalculateOnRewind.h"

#include <optional>

#include "AA_ObstacleDetectionComponent.generated.h"

class IAA_RacerContextProvider;
class AAA_WheeledVehiclePawn;
struct FAA_AIRacerContext;
class USplineComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALPINEASPHALT_API UAA_ObstacleDetectionComponent : public UActorComponent, public AA_RecalculateOnRewind
{
	GENERATED_BODY()

public:	
	UAA_ObstacleDetectionComponent();

#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisualLogEntry* Snapshot) const;
#endif // ENABLE_VISUAL_LOG

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void OnRewindBegin() override {}
	virtual void RecalculateOnRewind() override;

private:
	struct FThreatContext;

	void DoTick();

	void PopulateAllVehicles();

	bool IsPotentialThreat(const FAA_AIRacerContext& AIContext, const FThreatContext& ThreatContext, const AAA_WheeledVehiclePawn& CandidateVehicle) const;

	bool PopulateThreatContext(FThreatContext& ThreatContext) const;

	virtual UObject* AsUObject() override { return this;  }

	static float GetDistanceAlongSplineAtLocation(const USplineComponent& SplineComponent, const FVector& WorldLocation);

public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	mutable FOnVehicleObstaclesUpdated OnVehicleObstaclesUpdated {};

private:
	IAA_RacerContextProvider* RacerContextProvider{};

	UPROPERTY(Category = "Obstacles", EditAnywhere)
	float MaxDistanceThresholdMeters{ 100 };

	UPROPERTY(Transient)
	TArray<AAA_WheeledVehiclePawn*> DetectedVehicles{};

	UPROPERTY(Transient)
	TArray<AAA_WheeledVehiclePawn*> AllVehicles{};
};
