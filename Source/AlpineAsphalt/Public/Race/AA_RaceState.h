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

	FString ToString() const;
	float GetTotalDistance() const;
};

#pragma region Inline Definitions

inline float FAA_RaceState::GetTotalDistance() const
{
	return DistanceAlongSpline + LapCount * SplineLength;
}

#pragma endregion Inline Definitions
