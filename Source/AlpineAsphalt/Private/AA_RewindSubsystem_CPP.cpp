// Fill out your copyright notice in the Description page of Project Settings.


#include "AA_RewindSubsystem_CPP.h"
#include "AA_Rewindable_CPP.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(RewindSubsystem);

void UAA_RewindSubsystem_CPP::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(!RewindModeActive)
	{
		MaxRewindValue += DeltaTime;
		MaxRewindValue = FMath::Clamp(MaxRewindValue,0.f,MaxRecordingLength);
		UE_LOG(RewindSubsystem,Log,TEXT("MaxRewindTime: %f"), MaxRewindValue)
	}
}

TStatId UAA_RewindSubsystem_CPP::GetStatId() const
{
	return UObject::GetStatID();
}

void UAA_RewindSubsystem_CPP::RegisterRewindable(IAA_Rewindable_CPP* Rewindable)
{
	RewindableActors.Add(Rewindable);
}

void UAA_RewindSubsystem_CPP::Rewind(float AmountToRewind)
{
	CurrentRewindValue += AmountToRewind;
	CurrentRewindValue = FMath::Clamp(CurrentRewindValue,0.f,MaxRewindValue);
	UE_LOG(RewindSubsystem,Log,TEXT("CurrentRewindValue: %f"), CurrentRewindValue)

	for (const auto Rewindable : RewindableActors)
	{
		Rewindable->SetRewindTime(CurrentRewindValue);
	}
	CurrentRewindValueChangedDelegate.Broadcast(CurrentRewindValue/MaxRewindValue);

}

void UAA_RewindSubsystem_CPP::FastForward(float AmountToFastForward)
{
	CurrentRewindValue -= AmountToFastForward;
	CurrentRewindValue = FMath::Clamp(CurrentRewindValue,0.f,MaxRewindValue);
	UE_LOG(RewindSubsystem,Log,TEXT("CurrentRewindValue: %f"), CurrentRewindValue)

	for (const auto Rewindable : RewindableActors)
	{
		Rewindable->SetRewindTime(CurrentRewindValue);
	}
	CurrentRewindValueChangedDelegate.Broadcast(CurrentRewindValue/MaxRewindValue);

}

void UAA_RewindSubsystem_CPP::EnterRewindMode()
{
	for (const auto Actor : RewindableActors)
	{
		Actor->PauseRecordingSnapshots();
	}
	//Stop Time
	UGameplayStatics::SetGlobalTimeDilation(this,0.f);
	
	RewindModeActive = true;

	ActivatedDelegate.Broadcast();
}

void UAA_RewindSubsystem_CPP::CancelRewindMode()
{
	for (const auto Rewindable : RewindableActors)
	{
		Rewindable->SetRewindTime(0.f);
		Rewindable->ResumeRecordingSnapshots();
	}
	
	//Restart Time
	UGameplayStatics::SetGlobalTimeDilation(this,1.f);

	RewindModeActive = false;

	CurrentRewindValue = 0.f;

	DeactivatedDelegate.Broadcast();
}

void UAA_RewindSubsystem_CPP::ConfirmRewind()
{
	for (const auto Rewindable : RewindableActors)
	{
		Rewindable->ResumeRecordingSnapshots();
	}

	//Restart Time
	UGameplayStatics::SetGlobalTimeDilation(this,1.f);

	RewindModeActive = false;

	MaxRewindValue -= CurrentRewindValue;
	
	CurrentRewindValue = 0.f;

	DeactivatedDelegate.Broadcast();
}

bool UAA_RewindSubsystem_CPP::IsRewindModeActive()
{
	return RewindModeActive;
}
