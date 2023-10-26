// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/AA_MapCaptureSubsystem.h"

#include "UI/AA_MapCaptureActor.h"
#include "UI/MapCaptureComponent.h"

#include "Logging/LoggingUtils.h"
#include "Logging/AlpineAsphaltLogger.h"

void UAA_MapCaptureSubsystem::SetCaptureEnabled(bool bCaptureEnabled)
{
	if (!CaptureActor)
	{
		return;
	}

	check(CaptureActor->MapCaptureComponent);
	CaptureActor->MapCaptureComponent->SetCaptureEnabled(bCaptureEnabled);
}

void UAA_MapCaptureSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	UE_LOG(LogAlpineAsphalt, Log, TEXT("%s: OnWorldBeginPlay"), *GetName());

	Super::OnWorldBeginPlay(InWorld);

	FindAndSetCaptureActor(InWorld);
}

void UAA_MapCaptureSubsystem::FindAndSetCaptureActor(UWorld& InWorld)
{
	for (TObjectIterator<AAA_MapCaptureActor> It; It; ++It)
	{
		if (&InWorld != It->GetWorld())
		{
			continue;
		}

		if (!CaptureActor)
		{
			CaptureActor = *It;

			UE_LOG(LogAlpineAsphalt, Display, TEXT("%s: Found Map Capture Actor %s at %s"),
				*GetName(), *CaptureActor->GetName(), *CaptureActor->GetActorLocation().ToCompactString());
		}
		else
		{
			UE_LOG(LogAlpineAsphalt, Warning, TEXT("%s: Found duplicate Map Capture Actor %s at %s - Existing is %s at %s"),
				*GetName(), *It->GetName(), *It->GetActorLocation().ToCompactString(),
				*CaptureActor->GetName(), *CaptureActor->GetActorLocation().ToCompactString()
			);
		}
	}

	if (!CaptureActor)
	{
		UE_LOG(LogAlpineAsphalt, Error, TEXT("%s: Could not find map capture actor - no map will display for this level!"), *GetName());
	}
}
