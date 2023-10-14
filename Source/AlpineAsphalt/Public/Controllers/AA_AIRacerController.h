// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"

#include "AI/AA_RacerContextProvider.h"
#include "AI/AA_AIRacerContext.h"

#include "AA_AIRacerController.generated.h"

class UAA_AIVehicleControlComponent;
class UAA_RacerSplineFollowingComponent;
class UAA_ObstacleDetectionComponent;
class UAA_RacerObstacleAvoidanceComponent;

class AAA_WheeledVehiclePawn;
class ALandscape;

/**
 * 
 */
UCLASS()
class ALPINEASPHALT_API AAA_AIRacerController : public AAIController, public IAA_RacerContextProvider
{
	GENERATED_BODY()

public:
	AAA_AIRacerController();

	UFUNCTION(BlueprintCallable)
	UAA_AIVehicleControlComponent* GetVehicleControlComponent() const { return VehicleControlComponent; }

	// Inherited via IAA_RacerContextProvider
	virtual FAA_AIRacerContext& GetRacerContext() override;
	void SetTrackInfo(AAA_TrackInfoActor* TrackInfoActor);

#if ENABLE_VISUAL_LOG
	virtual void GrabDebugSnapshot(FVisualLogEntry* Snapshot) const override;
#endif

protected:

	virtual void BeginPlay() override;

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override final;

private:
	void SetupComponentEventBindings();
	void SetRaceTrack(const AAA_WheeledVehiclePawn& VehiclePawn);
	void SetVehicleParameters();

private:

	static constexpr double MaxRaceDistance = 500 * 100;

	UPROPERTY(Transient)
	FAA_AIRacerContext RacerContext{};

	UPROPERTY(Category = "Movement", VisibleDefaultsOnly)
	TObjectPtr<UAA_AIVehicleControlComponent> VehicleControlComponent{};

	UPROPERTY(Category = "Movement", VisibleDefaultsOnly)
	TObjectPtr<UAA_RacerSplineFollowingComponent> RacerSplineFollowingComponent{};

	UPROPERTY(Category = "Obstacles", VisibleDefaultsOnly)
	TObjectPtr<UAA_ObstacleDetectionComponent> ObstacleDetectionComponent{};

	UPROPERTY(Category = "Obstacles", VisibleDefaultsOnly)
	TObjectPtr<UAA_RacerObstacleAvoidanceComponent> RacerObstacleAvoidanceComponent{};
};

#pragma region Inline Definitions

inline FAA_AIRacerContext& AAA_AIRacerController::GetRacerContext()
{
	return RacerContext;
}

#pragma endregion Inline Definitions
