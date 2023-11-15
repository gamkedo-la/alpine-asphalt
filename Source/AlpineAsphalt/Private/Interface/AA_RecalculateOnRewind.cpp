// Fill out your copyright notice in the Description page of Project Settings.


#include "Interface/AA_RecalculateOnRewind.h"
#include "Subsystems/AA_RewindSubsystem.h"

void AA_RecalculateOnRewind::RegisterRewindCallback()
{
	if (Holder)
	{
		// already registered
		return;
	}

	auto Object = AsUObject();
	check(Object);

	auto World = Object->GetWorld();
	check(World);

	UAA_RewindSubsystem* RewindSystem = World->GetSubsystem<UAA_RewindSubsystem>();
	if (!RewindSystem)
	{
		return;
	}

	Holder = NewObject<UAA_RecalculateOnRewindDelegateHolder>();
	Holder->AddToRoot(); // Avoid spurious GC

	Holder->Register(*RewindSystem);
}

void AA_RecalculateOnRewind::UnregisterRewindCallback()
{
	if (!Holder)
	{
		return;
	}

	auto Object = AsUObject();
	check(Object);

	auto World = Object->GetWorld();
	if (World)
	{
		if (auto Subsystem = World->GetSubsystem<UAA_RewindSubsystem>(); Subsystem)
		{
			Holder->Deactivate(*Subsystem);
		}
	}

	Holder->RemoveFromRoot(); // Allow GC
	Holder = nullptr;
}

void UAA_RecalculateOnRewindDelegateHolder::Register(UAA_RewindSubsystem& Subsystem)
{
	Subsystem.DeactivatedDelegate.AddDynamic(this, &UAA_RecalculateOnRewindDelegateHolder::OnRewindModeDeactivated);
	Subsystem.CurrentRewindValueChangedDelegate.AddDynamic(this, &UAA_RecalculateOnRewindDelegateHolder::OnRewindPercentageChanged);
}

void UAA_RecalculateOnRewindDelegateHolder::Deactivate(UAA_RewindSubsystem& Subsystem)
{
	Subsystem.DeactivatedDelegate.RemoveDynamic(this, &UAA_RecalculateOnRewindDelegateHolder::OnRewindModeDeactivated);
	Subsystem.CurrentRewindValueChangedDelegate.RemoveDynamic(this, &UAA_RecalculateOnRewindDelegateHolder::OnRewindPercentageChanged);
}

void UAA_RecalculateOnRewindDelegateHolder::OnRewindModeDeactivated()
{
	if (bWasRewound)
	{
		check(Parent);
		Parent->RecalculateOnRewind();
	}
}

void UAA_RecalculateOnRewindDelegateHolder::OnRewindPercentageChanged(float Percentage)
{
	bWasRewound = true;
}
