// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include <concepts>
#include <type_traits>

#include "AA_BlueprintFunctionLibrary.generated.h"

class UChaosWheeledVehicleMovementComponent;
class UChaosVehicleWheel;
class AAA_TrackInfoActor;
class AAA_RoadSplineActor;
/**
 * 
 */
UCLASS()
class ALPINEASPHALT_API UAA_BlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

#if WITH_EDITOR

	UFUNCTION(BlueprintCallable,Category=LandscapeSpline)
	static void GenerateRoadSpline(class AActor* LandscapeSpline,TSubclassOf<AAA_RoadSplineActor> RoadSplineBP);

	UFUNCTION(BlueprintCallable,Category=RaceSpline)
	static void GenerateRaceSpline(TArray<class AActor*> RoadSplines, TSubclassOf<AAA_TrackInfoActor> RaceSplineBP);
#endif

	UFUNCTION(BlueprintCallable, Category=ChaosWheel)
	static UPhysicalMaterial* GetWheelContactMaterial(UChaosVehicleWheel* Wheel);

	/*
	* Wraps between [Min,Max].  This version of wrap with proper behavior.  FMath::Wrap has a bug currently.
	* See https://forums.unrealengine.com/t/fmath-wrap-behavior/266612.
	* 
	* We use <code>std::remove_cvref_t<T></code> to remove any const/volatile qualifiers during type deduction so that caller doesn't need to specify template argument if one happens to be "const".
	*/
	template<typename T> requires std::is_arithmetic_v<std::remove_cvref_t<T>>
	static T WrapEx(T Value, T Min, T Max);

	/*
	* Wraps an integer betwen [Min,Max].  This version of wrap with proper behavior.  FMath::Wrap has a bug currently.
	* See https://forums.unrealengine.com/t/fmath-wrap-behavior/266612.
	*/
	UFUNCTION(BlueprintPure, meta = (DisplayName = "WrapEx (Integer)", Min = "0", Max = "100"), Category = "Math|Integer")
	static int32 Wrap(int32 Value, int32 Min, int32 Max);
};

#pragma region Template Definitions

template<typename T> requires std::is_arithmetic_v<std::remove_cvref_t<T>>
T UAA_BlueprintFunctionLibrary::WrapEx(T Value, T Min, T Max)
{
	if (Min == Max)
	{
		return Min;
	}

	const auto Size = Max - Min + 1;

	auto EndVal{ Value };

	while (EndVal < Min)
	{
		EndVal += Size;
	}

	while (EndVal > Max)
	{
		EndVal -= Size;
	}

	return EndVal;
}

#pragma endregion Template Definitions

#pragma region Inline Definitions

inline int32 UAA_BlueprintFunctionLibrary::Wrap(int32 Value, int32 Min, int32 Max)
{
	return WrapEx(Value, Min, Max);
}

#pragma endregion Inline Definitions
