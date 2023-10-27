// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"

#include "AI/AA_RacerContextProvider.h"
#include "AI/AA_AIRacerContext.h"
#include "AI/AA_AIDifficulty.h"

#include <optional>

#include "AA_AIRacerController.generated.h"

class UAA_AIVehicleControlComponent;
class UAA_RacerSplineFollowingComponent;
class UAA_ObstacleDetectionComponent;
class UAA_RacerObstacleAvoidanceComponent;
class UAA_AIGetUnstuckComponent;

class AAA_WheeledVehiclePawn;
class ALandscape;


USTRUCT(BlueprintType)
struct ALPINEASPHALT_API FAA_RacerAISettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	EAA_AIDifficulty Difficulty{ EAA_AIDifficulty::Easy };

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	float MinSpeedMph{ 15.0f };

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	float MaxSpeedMph{ 50.0f };

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	bool bEnableABS{ false };

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	bool bEnableTractionControl{ false };

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (ClampMin = 1.0f))
	float BrakingForceBoostMultiplier{ 1.0f };

	/*
	* Setting to 0 makes it easier for the AI to drive and 1 makes it harder.
	*/
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	float WheelLoadRatio{ 1.0f };

	FString ToString() const;
};

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
	void SetVehicleParameters(const FAA_RacerAISettings& RacerAISettings);

	FAA_RacerAISettings GetCurrentRacerAISettings() const;

	UFUNCTION()
	void OnRacerSettingsUpdated();

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

	UPROPERTY(Category = "Movement", VisibleDefaultsOnly)
	TObjectPtr<UAA_AIGetUnstuckComponent> GetUnstuckComponent{};

	UPROPERTY(Category = "Difficulty", EditDefaultsOnly, EditFixedSize, meta = (TitleProperty = "Difficulty"))
	TArray<FAA_RacerAISettings> DifficultySettings{};

	std::optional<EAA_AIDifficulty> CurrentDifficulty{};
};

#pragma region Inline Definitions

inline FAA_AIRacerContext& AAA_AIRacerController::GetRacerContext()
{
	return RacerContext;
}

#pragma endregion Inline Definitions
