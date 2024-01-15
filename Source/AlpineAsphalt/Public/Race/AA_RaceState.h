// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "AA_RaceState.generated.h"

class AAA_WheeledVehiclePawn;
class AAA_TrackInfoActor;

USTRUCT(BlueprintType)
struct ALPINEASPHALT_API FAA_RaceState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	TObjectPtr<const AAA_TrackInfoActor> RaceTrack{};

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	TObjectPtr<const AAA_WheeledVehiclePawn> VehiclePawn{};

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	float DistanceAlongSpline{};
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	float MaxDistanceAlongSpline{};

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	float SplineLength{};

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient)
	int32 LapCount{};

	// Keep track of max progress on current lap to avoid cheating by going backwards to increment the lap counter
	float CurrentLapMaxCompletionFraction{};

	FString ToString() const;
	float GetTotalDistance() const;
	float GetCurrentLapCompletionFraction() const;
	float GetOverallCompletionFraction() const;
	float GetLapsFraction() const;
	int32 GetNumLaps() const;
	bool IsOnLastLap() const;
	bool IsLooping() const;

	bool IsValid() const;
	operator bool() const;
};

struct ALPINEASPHALT_API FAA_RaceStateSnapshotData
{
	float DistanceAlongSpline{};
	float CurrentLapMaxCompletionFraction{};
	int32 LapCount{};
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

inline float FAA_RaceState::GetOverallCompletionFraction() const
{
	return GetLapsFraction() / GetNumLaps();
}

inline bool FAA_RaceState::IsOnLastLap() const
{
	return !IsLooping() || LapCount == GetNumLaps() - 1;
}

inline bool FAA_RaceState::IsValid() const
{
	return RaceTrack && VehiclePawn;
}

inline FAA_RaceState::operator bool() const
{
	return IsValid();
}

#pragma endregion Inline Definitions
