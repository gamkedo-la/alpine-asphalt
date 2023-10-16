// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "AI/AA_AIRacerEvents.h"

#include <optional>

#include "AA_RacerSplineFollowingComponent.generated.h"

class AAA_WheeledVehiclePawn;
class IAA_RacerContextProvider;
struct FAA_AIRacerContext;

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

	UFUNCTION()
	void SelectUnstuckTarget(AAA_WheeledVehiclePawn* VehiclePawn, const FVector& IdealSeekPosition);

#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisualLogEntry* Snapshot) const;
#endif // ENABLE_VISUAL_LOG

protected:
	virtual void BeginPlay() override;

private:
	// TODO: May respond to a race start event
	void SetInitialMovementTarget();

	struct FSplineState
	{
		FVector OriginalWorldLocation;
		FVector WorldLocation;
		FVector SplineDirection;
		float SplineKey;
		float DistanceAlongSpline;	
	};

	std::optional<FSplineState> GetInitialSplineState(const FAA_AIRacerContext& RacerContext) const;
	std::optional<FSplineState> GetNextSplineState(const FAA_AIRacerContext& RacerContext, std::optional<float> NextDistanceAlongSplineOverride = {}) const;

	void UpdateMovementFromLastSplineState(FAA_AIRacerContext& RacerContext);

	/*
	* Curvature between [0,1] indicating how steep the upcoming road is for speed adjustment purposes.
	*/
	float CalculateUpcomingRoadCurvature() const;

public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	mutable FOnVehicleTargetUpdated OnVehicleTargetUpdated {};

private:
	IAA_RacerContextProvider* RacerContextProvider{};

	std::optional<FSplineState> LastSplineState{};

	UPROPERTY(Category = "Movement", EditAnywhere)
	float LookaheadDistance{ 1000.0f };

	UPROPERTY(Category = "Movement", EditAnywhere)
	float MaxLookaheadDistance{ 4000.0f };

	UPROPERTY(Category = "Movement", EditAnywhere)
	float MinLookaheadDistance{ 300.0f };

	UPROPERTY(Category = "Movement", EditAnywhere)
	float MinSpeedMph{ 20.0f };

	UPROPERTY(Category = "Movement", EditAnywhere)
	float MaxSpeedMph{ 80.0f };

	UPROPERTY(Category = "Movement", EditAnywhere)
	float RoadCurvatureLookaheadFactor{ 3.0f };

	/*
	* Balance between curvature and speed for setting the lookahead target.
	* Values range [0,1].
	* As curvature and speed increases, the lookahead distance increases.
	*/
	UPROPERTY(Category = "Movement", EditAnywhere)
	float LookaheadCurvatureAlphaWeight{ 0.5f };

	UPROPERTY(Category = "Movement", VisibleInstanceOnly)
	float LastCurvature{ 1.0f };

	UPROPERTY(Category = "Movement", VisibleInstanceOnly)
	float CurrentOffset{ 0.0f };
};
