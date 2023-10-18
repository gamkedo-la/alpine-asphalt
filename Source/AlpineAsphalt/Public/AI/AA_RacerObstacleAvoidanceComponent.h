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

	struct FThreatResult
	{
		FVector ThreatVector;
		double Score;
	};

	std::optional<FThreatResult> ComputeThreatResult(const FThreatContext& ThreatContext, const AAA_WheeledVehiclePawn& CandidateVehicle) const;

	bool PopulateThreatContext(FThreatContext& ThreatContext) const;

	float GetAverageSpeed(const FThreatContext& ThreatContext, const AAA_WheeledVehiclePawn& CandidateVehicle, float ThreatDistance) const;

	static const FAA_AIRacerContext* GetRacerAIContext(const AAA_WheeledVehiclePawn& CandidateVehicle);
	static bool WillBeAccelerating(const AAA_WheeledVehiclePawn& CandidateVehicle, const FAA_AIRacerContext* AIContext);
	static bool WillBeBraking(const AAA_WheeledVehiclePawn& CandidateVehicle, const FAA_AIRacerContext* AIContext);

private:
	IAA_RacerContextProvider* RacerContextProvider{};

	/*
	* If potential threat is <= this amount of car lengths then consider the threat no matter what the speed and alignment.
	*/
	UPROPERTY(Category = "Avoidance", EditAnywhere)
	float MinThreatSpeedCarLengthsDistance{ 2.0f };

	/*
	* Rough estimate for acceleration values in cm/s^2.  Assume an average racing car with 0-60mph in 3s.
	* V = Vo + at -> a = V / t = (60mph)/(3s) = 894 cm/s^2
	* e.g. https://www.drivinggeeks.com/fast-zero-60-time/
	*/
	UPROPERTY(Category = "Avoidance", EditAnywhere, meta = (ClampMin = "1.0"))
	float AverageAcceleration{ 900 };

	/**
	* Rough estimate for deceleration values in cm/s^2 due to braking.  Assume an average racing car with 60-0mph in 100 ft.
	* V^2 = Vo^2 + 2ax -> a = -Vo^2/2*x = -(60mph)^2/(2 * 100 ft) = -1180 cm/s^2
	* e.g https://www.motortrend.com/features/20-best-60-to-0-distances-recorded/
	*/
	UPROPERTY(Category = "Avoidance", EditAnywhere, meta = (ClampMin = "0.0"))
	float AverageDeceleration { 1200 };
};

