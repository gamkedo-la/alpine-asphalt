// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "AI/AA_AIRacerEvents.h"
#include "AI/AA_AIRacerContext.h"

#include <optional>

#include "AA_RacerSplineFollowingComponent.generated.h"

class AAA_WheeledVehiclePawn;
class IAA_RacerContextProvider;
class ALandscape;

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

	UFUNCTION(BlueprintCallable, Category = "Racer AI")
	void SetMaxSpeedMph(float SpeedMph);

	UFUNCTION(BlueprintCallable, Category = "Racer AI")
	void SetMinSpeedMph(float SpeedMph);

#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisualLogEntry* Snapshot) const;
#endif // ENABLE_VISUAL_LOG

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

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
		float RoadOffset{};
		float LookaheadDistance;

		FString ToString() const;
	};

	std::optional<FSplineState> GetInitialSplineState(const FAA_AIRacerContext& RacerContext) const;
	std::optional<FSplineState> GetNextSplineState(const FAA_AIRacerContext& RacerContext, std::optional<float> NextDistanceAlongSplineOverride = {}, std::optional<float> LookaheadDistanceOverride = {}) const;
	void UpdateSplineStateWithRoadOffset(const FAA_AIRacerContext& RacerContext, FSplineState& SplineState, float RoadOffset) const;

	void UpdateMovementFromLastSplineState(FAA_AIRacerContext& RacerContext);
	void UpdateLastSplineStateIfApproachTooSteep(const FAA_AIRacerContext& RacerContext, const AAA_WheeledVehiclePawn& VehiclePawn, const FSplineState& OriginalSplineState);

	/*
	* Curvature between [0,1] indicating how steep the upcoming road is for speed adjustment purposes.
	*/
	float CalculateUpcomingRoadCurvature() const;

	float CalculateMaxOffsetAtLastSplineState() const;

	void ResetAvoidanceContext();

	float CalculateNewSpeed(const FAA_AIRacerContext& RacerContext) const;

	float ClampSpeed(float Speed) const;

	bool IsSplineStateASufficientTarget(const AAA_WheeledVehiclePawn& VehiclePawn, const FSplineState& SplineState) const;

	ALandscape* GetLandscapeActor() const;
	bool IsCurrentPathOccluded(const AAA_WheeledVehiclePawn& VehiclePawn, const FSplineState& SplineState) const;
public:
	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	mutable FOnVehicleTargetUpdated OnVehicleTargetUpdated {};

	UPROPERTY(Category = "Notification", Transient, BlueprintAssignable)
	mutable FOnRaceCompleted OnRaceCompleted {};

private:
	IAA_RacerContextProvider* RacerContextProvider{};

	std::optional<FSplineState> LastSplineState{};
	std::optional<FAA_AIRacerAvoidanceContext> LastAvoidanceContext{};

	UPROPERTY(Category = "Movement", EditAnywhere)
	float MaxLookaheadDistance{ 4000.0f };

	UPROPERTY(Category = "Movement", EditAnywhere)
	float MinLookaheadDistance{ 300.0f };

	UPROPERTY(Transient, Category = "Movement", VisibleInstanceOnly)
	float MinSpeedMph{ 20.0f };

	UPROPERTY(Transient, Category = "Movement", VisibleInstanceOnly)
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


	/**
	*  Reduce speed by this factor for each avoidance threat detected.
	*/
	UPROPERTY(Category = "Movement", EditAnywhere)
	float AvoidanceThreatSpeedReductionFactor{ 1.0f / 6 };

	UPROPERTY(Category = "Movement", EditAnywhere)
	float CurvatureRoadOffsetBlendFactor{ 0.5f };

	UPROPERTY(Transient, Category = "Movement", VisibleInstanceOnly)
	float LastCurvature{ 0.0f };

	FVector LastMovementTarget{ EForceInit::ForceInitToZero };

	UPROPERTY(Transient, Category = "Movement", VisibleInstanceOnly)
	float CurrentAvoidanceOffset{ 0.0f };

	UPROPERTY(Category = "Movement", EditAnywhere)
	float MinInitialTargetAlignment{ 0.2f };

	UPROPERTY(Category = "Movement", EditDefaultsOnly)
	float MaxApproachAngle{ 22.5f };

	float MaxApproachAngleCosine{};

	UPROPERTY(Transient)
	ALandscape* Landscape{};
};

#pragma region Inline Definitions

inline float UAA_RacerSplineFollowingComponent::ClampSpeed(float Speed) const
{
	return FMath::Clamp(Speed, MinSpeedMph, MaxSpeedMph);
}

inline void UAA_RacerSplineFollowingComponent::SetMaxSpeedMph(float SpeedMph)
{
	MaxSpeedMph = SpeedMph;
}

inline void UAA_RacerSplineFollowingComponent::SetMinSpeedMph(float SpeedMph)
{
	MinSpeedMph = SpeedMph;
}

#pragma endregion
