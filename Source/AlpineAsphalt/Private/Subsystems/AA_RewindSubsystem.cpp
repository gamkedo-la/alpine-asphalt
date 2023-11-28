// Fill out your copyright notice in the Description page of Project Settings.


#include "Subsystems/AA_RewindSubsystem.h"
#include "Interface/AA_RewindableInterface.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY(RewindSubsystem);

void UAA_RewindSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	RewindStartTime = InWorld.GetTimeSeconds();
}

float UAA_RewindSubsystem::GetMaxRewindTime()
{
	float ElapsedTime = GetWorld()->GetTimeSeconds() - RewindStartTime;
	float MaxTime = FMath::Clamp(ElapsedTime,0,MaxRecordingLength);
	return MaxTime;
}

void UAA_RewindSubsystem::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	/*
	if(!RewindModeActive)
	{
		MaxRewindValue += DeltaTime;
		MaxRewindValue = FMath::Clamp(MaxRewindValue,0.f,MaxRecordingLength);
		UE_LOG(RewindSubsystem,Verbose,TEXT("MaxRewindTime: %f"), MaxRewindValue)
	}
	*/
}

TStatId UAA_RewindSubsystem::GetStatId() const
{
	return UObject::GetStatID();
}

void UAA_RewindSubsystem::RegisterRewindable(IAA_RewindableInterface* Rewindable)
{
	RewindableActors.Add(Rewindable);
}
void UAA_RewindSubsystem::UnregisterRewindable(IAA_RewindableInterface* Rewindable)
{
	RewindableActors.Remove(Rewindable);
}

void UAA_RewindSubsystem::Rewind(float AmountToRewind)
{
	float MaxRewindValue = GetMaxRewindTime();
	CurrentRewindValue += AmountToRewind;
	CurrentRewindValue = FMath::Clamp(CurrentRewindValue,0.f,MaxRewindValue);
	UE_LOG(RewindSubsystem,Verbose,TEXT("CurrentRewindValue: %f"), CurrentRewindValue)

	for (const auto Rewindable : RewindableActors)
	{
		Rewindable->SetRewindTime(CurrentRewindValue);
	}
	CurrentRewindValueChangedDelegate.Broadcast(CurrentRewindValue/MaxRewindValue);

}

void UAA_RewindSubsystem::FastForward(float AmountToFastForward)
{
	float MaxRewindValue = GetMaxRewindTime();
	CurrentRewindValue -= AmountToFastForward;
	CurrentRewindValue = FMath::Clamp(CurrentRewindValue,0.f,MaxRewindValue);
	UE_LOG(RewindSubsystem,Verbose,TEXT("CurrentRewindValue: %f"), CurrentRewindValue)

	for (const auto Rewindable : RewindableActors)
	{
		Rewindable->SetRewindTime(CurrentRewindValue);
	}
	CurrentRewindValueChangedDelegate.Broadcast(CurrentRewindValue/MaxRewindValue);

}

bool UAA_RewindSubsystem::EnterRewindMode()
{
	if(!CanEnterRewindMode())
	{
		return false;
	}
	for (const auto Actor : RewindableActors)
	{
		Actor->PauseRecordingSnapshots();
	}
	//Stop Time
	UGameplayStatics::SetGlobalTimeDilation(this,0.f);
	
	RewindModeActive = true;

	ActivatedDelegate.Broadcast();

	return true;
}

void UAA_RewindSubsystem::CancelRewindMode()
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

void UAA_RewindSubsystem::ConfirmRewind()
{
	for (const auto Rewindable : RewindableActors)
	{
		Rewindable->ResumeRecordingSnapshots();
	}

	//Restart Time
	UGameplayStatics::SetGlobalTimeDilation(this,1.f);

	RewindModeActive = false;

	RewindStartTime += CurrentRewindValue;
	
	CurrentRewindValue = 0.f;

	DeactivatedDelegate.Broadcast();
}

void UAA_RewindSubsystem::ResetRewindHistory()
{
	for (const auto Rewindable : RewindableActors)
	{
		Rewindable->ResetRewindHistory();
	}
	RewindStartTime = GetWorld()->GetTimeSeconds();
}

bool UAA_RewindSubsystem::IsRewindModeActive() const
{
	return RewindModeActive;
}

bool UAA_RewindSubsystem::CanEnterRewindMode() const
{
	return RewindModeEnabled && !RewindModeActive;
}

void UAA_RewindSubsystem::EnableRewindMode(bool bisEnabled)
{
	RewindModeEnabled = bisEnabled;
}
