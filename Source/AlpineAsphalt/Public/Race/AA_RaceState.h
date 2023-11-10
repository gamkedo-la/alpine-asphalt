// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "AA_RaceState.generated.h"

class AAA_WheeledVehiclePawn;

USTRUCT(BlueprintType)
struct ALPINEASPHALT_API FAA_RaceState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	TObjectPtr<const AAA_WheeledVehiclePawn> VehiclePawn{};

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	float DistanceAlongSpline{};

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	float SplineLength{};

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	int32 LapCount{};

	// Keep track of max progress on current lap to avoid cheating by going backwards to increment the lap counter
	float CurrentLapMaxCompletionFraction{};

	FString ToString() const;
	float GetTotalDistance() const;
	float GetCurrentLapCompletionFraction() const;
	float GetLapsFraction() const;
};

#pragma region Inline Definitions

inline float FAA_RaceState::GetTotalDistance() const
{
	return DistanceAlongSpline + LapCount * SplineLength;
}

inline float FAA_RaceState::GetCurrentLapCompletionFraction() const
{
	return DistanceAlongSpline / SplineLength;
}

inline float FAA_RaceState::GetLapsFraction() const
{
	return LapCount + GetCurrentLapCompletionFraction();
}

#pragma endregion Inline Definitions
