// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Interface/AA_RewindableInterface.h"
#include "Subsystems/AA_RewindSubsystem.h"

/**
 * Templated base class to handle common functionality with rewinding.
 * Only need to implement the two pure virtual functions CaptureSnapshot and RestoreFromSnapshot.
 * RegisterRewindable in <c>BeginPlay</c> and UnregisterRewindable in <c>EndPlay</c>.
 */
template<typename TSnapshotData>
class ALPINEASPHALT_API TAA_BaseRewindable : public IAA_RewindableInterface
{
public:
	virtual void SetRewindTime(float Time) override final;
	virtual void PauseRecordingSnapshots() override final;
	virtual void ResumeRecordingSnapshots() override final;
	virtual void ResetRewindHistory() override final;

	virtual ~TAA_BaseRewindable() = default;

protected:
	void RegisterRewindable();
	void UnregisterRewindable();

	virtual TSnapshotData CaptureSnapshot() const = 0;
	virtual void RestoreFromSnapshot(const TSnapshotData& InSnapshotData) = 0;
	virtual UObject* AsUObject() = 0;

private:
	void RecordSnapshot();

private:
	FTimerHandle RecordingSnapshotTimerHandle;
	TArray<TSnapshotData> SnapshotData{};

	//Cached property of Rewind Subsystem
	int32 MaxSnapshots = 0;
	//Cached Time that should match RewindSubsystem
	float RewindTime = 0;
	//Cached property of Rewind Subsystem
	float RewindResolution = 0.f;

	bool bSnapshotsPaused = false;
};

#pragma region Template Definitions

template<typename TSnapshotData>
void TAA_BaseRewindable<TSnapshotData>::RegisterRewindable()
{
	auto Object = AsUObject();
	check(Object);

	auto World = Object->GetWorld();
	check(World);
	
	UAA_RewindSubsystem* RewindSystem = World->GetSubsystem<UAA_RewindSubsystem>();
	if (!RewindSystem)
	{
		return;
	}

	World->GetTimerManager().SetTimer(
		RecordingSnapshotTimerHandle,
		FTimerDelegate::CreateRaw(this, &TAA_BaseRewindable<TSnapshotData>::RecordSnapshot),
		RewindSystem->RecordingResolution, true);
		
	MaxSnapshots = FMath::FloorToInt(RewindSystem->MaxRecordingLength / RewindSystem->RecordingResolution);
	RewindResolution = RewindSystem->RecordingResolution;

	SnapshotData.Reserve(MaxSnapshots);

	RewindSystem->RegisterRewindable(this);
}

template<typename TSnapshotData>
void TAA_BaseRewindable<TSnapshotData>::UnregisterRewindable()
{
	auto Object = AsUObject();
	check(Object);

	auto World = Object->GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(RecordingSnapshotTimerHandle);

	if (auto Subsystem = World->GetSubsystem<UAA_RewindSubsystem>())
	{
		Subsystem->UnregisterRewindable(this);
	}
}

template<typename TSnapshotData>
void TAA_BaseRewindable<TSnapshotData>::SetRewindTime(float Time)
{
	RewindTime = Time;

	int32 Index = FMath::FloorToInt(RewindTime / RewindResolution);
	Index = SnapshotData.Num() - (Index + 1);
	Index = Index >= 0 ? Index : 0;

	if (Index >= SnapshotData.Num())
	{
		return;
	}

	RestoreFromSnapshot(SnapshotData[Index]);
}

template<typename TSnapshotData>
void TAA_BaseRewindable<TSnapshotData>::PauseRecordingSnapshots()
{
	RecordSnapshot();

	auto Object = AsUObject();
	check(Object);

	auto World = Object->GetWorld();
	check(World);

	World->GetTimerManager().PauseTimer(RecordingSnapshotTimerHandle);
	bSnapshotsPaused = true;
}

template<typename TSnapshotData>
void TAA_BaseRewindable<TSnapshotData>::ResumeRecordingSnapshots()
{
	int index = FMath::FloorToInt(RewindTime / RewindResolution);
	index = SnapshotData.Num() - (index + 1);
	if (index < SnapshotData.Num())
	{
		//reverse through list
		for (int i = SnapshotData.Num() - 1; i > index; i--)
		{
			// don't deallocate memory
			SnapshotData.RemoveAt(i, 1, false);
		}
	}

	bSnapshotsPaused = false;

	auto Object = AsUObject();
	check(Object);

	auto World = Object->GetWorld();
	check(World);

	World->GetTimerManager().UnPauseTimer(RecordingSnapshotTimerHandle);
}

template<typename TSnapshotData>
void TAA_BaseRewindable<TSnapshotData>::ResetRewindHistory()
{
	SnapshotData.Reset();
}

template<typename TSnapshotData>
void TAA_BaseRewindable<TSnapshotData>::RecordSnapshot()
{
	if (bSnapshotsPaused) 
	{ 
		return; 
	}

	// TODO: Consider using a circular buffer
	if (SnapshotData.Num() > MaxSnapshots)
	{
		SnapshotData.RemoveAt(0, 1, false);
	}

	SnapshotData.Add(CaptureSnapshot());
}

#pragma endregion Template Definitions
