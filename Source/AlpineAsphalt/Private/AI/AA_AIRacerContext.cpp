// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AA_AIRacerContext.h"

FString FAA_AIRacerAvoidanceContext::ToString() const
{
	return FString::Printf(TEXT("NormalizedThreatScore=%f; ThreatCount=%d; ThreatVector=%s; NormalizedThreatSpeedMph=%f"),
		NormalizedThreatScore, ThreatCount, *ThreatVector.ToCompactString(), NormalizedThreatSpeedMph);
}

void FAA_AIRacerContext::SetVehiclePawn(AAA_WheeledVehiclePawn* InVehiclePawn)
{
	RaceState.VehiclePawn = VehiclePawn = InVehiclePawn;
}

void FAA_AIRacerContext::SetRaceTrack(AAA_TrackInfoActor* InRaceTrack)
{
	RaceState.RaceTrack = RaceTrack = InRaceTrack;
}
