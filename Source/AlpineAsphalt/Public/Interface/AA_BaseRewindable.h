// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Interface/AA_RewindableInterface.h"
#include "Interface/AA_RecalculateOnRewind.h"
#include "Subsystems/AA_RewindSubsystem.h"

/**
 * Templated base class to handle common functionality with rewinding.
 * Only need to implement the two pure virtual functions CaptureSnapshot and RestoreFromSnapshot.
 * RegisterRewindable in <c>BeginPlay</c> and UnregisterRewindable in <c>EndPlay</c>.
 */
template<typename TSnapshotData>
class ALPINEASPHALT_API TAA_BaseRewindable : public IAA_RewindableInterface, private AA_RecalculateOnRewind
{
public:

	enum class ERestoreTiming : uint8
	{
		/*
		* Immediately restore the snapshot as soon as the rewind occurs.  Needed for visual components.
		*/
		Immediate,

		/*
		* Restore once play resumes.  Usually for calculating non-visual components.
		*/
		Resume
	};

	virtual void SetRewindTime(float Time) override final;
	virtual void PauseRecordingSnapshots() override final;
	virtual void ResumeRecordingSnapshots() override final;
	virtual void ResetRewindHistory() override final;

	virtual ~TAA_BaseRewindable() = default;

protected:
	void RegisterRewindable(ERestoreTiming InRestoreTiming);
	void UnregisterRewindable();

	virtual TSnapshotData CaptureSnapshot() const = 0;
	virtual void RestoreFromSnapshot(const TSnapshotData& InSnapshotData, float InRewindTime) = 0;
	virtual UObject* AsUObject() = 0;

	// optional functions to override
	// Inherited from AA_RecalculateOnRewind
	virtual void OnRewindBegin() override {}

private:
	void RecordSnapshot();

	// Inherited from AA_RecalculateOnRewind
	virtual void RecalculateOnRewind() override;

private:
	FTimerHandle RecordingSnapshotTimerHandle;
	TArray<TSnapshotData> SnapshotData{};

	//Cached property of Rewind Subsystem
	int32 MaxSnapshots = 0;
	//Cached Time that should match RewindSubsystem
	float RewindTime = 0;
	//Cached property of Rewind Subsystem
	float RewindResolution = 0.f;

	int32 LastSnapshotIndex{ -1 };

	bool bSnapshotsPaused = false;
	ERestoreTiming RestoreTiming{};
};

#pragma region Template Definitions

template<typename TSnapshotData>
void TAA_BaseRewindable<TSnapshotData>::RegisterRewindable(ERestoreTiming InRestoreTiming)
{
	auto Object = AsUObject();
	check(Object);

	auto World = Object->GetWorld();
	check(World);
	
	RestoreTiming = InRestoreTiming;

	UAA_RewindSubsystem* RewindSystem = World->GetSubsystem<UAA_RewindSubsystem>();
	if (!RewindSystem)
	{
		return;
	}

	// register AA_RecalculateOnRewind behavior
	if (RestoreTiming == ERestoreTiming::Resume)
	{
		RegisterRewindCallback();
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

	if (RestoreTiming == ERestoreTiming::Resume)
	{
		UnregisterRewindCallback();
	}

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

	LastSnapshotIndex = Index;

	if (RestoreTiming == ERestoreTiming::Immediate)
	{
		RestoreFromSnapshot(SnapshotData[Index], RewindTime);
	}
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
	LastSnapshotIndex = -1;
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

template<typename TSnapshotData>
void TAA_BaseRewindable<TSnapshotData>::RecalculateOnRewind()
{
	// Rewind history reset or rewind mode canceled
	if (LastSnapshotIndex < 0 || LastSnapshotIndex >= SnapshotData.Num() || FMath::IsNearlyZero(RewindTime))
	{
		return;
	}

	// We only register this callback for resume case
	check(RestoreTiming == ERestoreTiming::Resume);

	RestoreFromSnapshot(SnapshotData[LastSnapshotIndex], RewindTime);
}

#pragma endregion Template Definitions
