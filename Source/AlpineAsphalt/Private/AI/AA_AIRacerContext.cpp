// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AA_AIRacerContext.h"

FString FAA_AIRacerAvoidanceContext::ToString() const
{
	return FString::Printf(TEXT("NormalizedThreatScore=%f; ThreatCount=%d; ThreatVector=%s"),
		NormalizedThreatScore, ThreatCount, *ThreatVector.ToCompactString());
}
