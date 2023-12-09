#include "Activity/AA_TimeTrialActivity.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Actors/AA_Checkpoint.h"
#include "Actors/AA_TrackInfoActor.h"
#include "Components/AA_ChaosWheeledVehicleMovementComponent.h"
#include "Components/AA_CheckpointComponent.h"
#include "Components/SplineComponent.h"
#include "Controllers/AA_PlayerController.h"
#include "Engine/Internal/Kismet/BlueprintTypeConversions.h"
#include "Kismet/GameplayStatics.h"
#include "Pawn/AA_WheeledVehiclePawn.h"
#include "Subsystems/AA_ActivityManagerSubsystem.h"
#include "UI/AA_BaseUI.h"
#include "UI/AA_TimeTrialScoreScreenUI.h"
#include "UI/AA_VehicleUI.h"

void UAA_TimeTrialActivity::Initialize(AAA_TrackInfoActor* TrackToUse)
{
	if(!TrackToUse)
	{
		UE_LOG(LogTemp,Error,TEXT("Couldn't Initialize TimeTrialActivity: Track was nullptr"))
		return;
	}
	this->Track = TrackToUse;
	NumCheckpoints = Track->CheckpointComponent->CheckpointPositionData.Num();
	LastCheckpointHitIndex = -1;
}

void UAA_TimeTrialActivity::LoadActivity()
{
	//Start Replay
	//UGameplayStatics::GetGameInstance(this)->StartRecordingReplay(FString("Replay"),FString("Replay"));
	
	//Spawn DataLayers, Despawn DataLayers, Spawn Checkpoints
	Track->LoadRace();
	
	//Listen to Checkpoints
	for (const auto Checkpoint : Track->CheckpointComponent->SpawnedCheckpoints)
	{
		Checkpoint->CheckpointHit.AddDynamic(this, &UAA_TimeTrialActivity::CheckpointHit);
	}

	//Move Player Vehicle to Start Position
	auto PlayerVehicle = Cast<AAA_WheeledVehiclePawn>(UGameplayStatics::GetPlayerPawn(GetWorld(),0));
	auto Rotation = Track->StartLocations[0].Rotator() + Track->GetActorRotation();
	auto Location = Track->GetActorRotation().RotateVector(Track->StartLocations[0].GetLocation());
	Location += Track->GetActorLocation();
	PlayerVehicle->ResetVehicleAtLocation(Location, Rotation);

	//TODO: Lock Car in Place
	PlayerVehicle->GetVehicleMovementComponent()->SetParked(true);
	
	GetWorld()->Exec(GetWorld(),TEXT("demo.MinRecordHz 60"));
	GetWorld()->Exec(GetWorld(),TEXT("demo.RecordHz 120"));

	auto PlayerController = Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	PlayerController->SetTrackInfo(Track);

	GetWorld()->GetSubsystem<UAA_ActivityManagerSubsystem>()->OnLoadActivityCompleted.Broadcast();
}

bool UAA_TimeTrialActivity::IsPlayerCompleted() const
{
	return bScoreScreenActive;
}

void UAA_TimeTrialActivity::UpdatePlayerHUD() const
{
	// Get Player Info
	auto PC = Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (!PC)
	{
		return;
	}
	const auto& PlayerSplineInfo = PC->GetPlayerSplineInfo();
	if (!PlayerSplineInfo)
	{
		return;
	}
	const auto& PlayerRaceState = PlayerSplineInfo->RaceState;
	
	//Update Missed Checkpoint HUD
	if(Track->CheckpointComponent->SpawnedCheckpoints.Num() > LastCheckpointHitIndex + 1)
	{
		const FVector NextCheckpointLocation = Track->CheckpointComponent->SpawnedCheckpoints[LastCheckpointHitIndex+1]->GetActorLocation();
		const float SplineInputKey = Track->Spline->FindInputKeyClosestToWorldLocation(NextCheckpointLocation);
		const float CheckpointDistanceAlongSpline = Track->Spline->GetDistanceAlongSplineAtSplineInputKey(SplineInputKey);
		bool PlayerMissedCheckpoint = PlayerRaceState.DistanceAlongSpline > CheckpointDistanceAlongSpline;
		//if the checkpoint we last hit is the last one,
		//we could be past the last checkpoint, but still at the end of the spline numerically
		//and the next spline would be the first one which will have a low value.
		if(PlayerMissedCheckpoint && LastCheckpointHitIndex == Track->CheckpointComponent->SpawnedCheckpoints.Num()-1)
		{
			const FVector LastCheckpointLocation = Track->CheckpointComponent->SpawnedCheckpoints.Last()->GetActorLocation();
			const float LastInputKey = Track->Spline->FindInputKeyClosestToWorldLocation(LastCheckpointLocation);
			const float LastCheckpointDistanceAlongSpline = Track->Spline->GetDistanceAlongSplineAtSplineInputKey(LastInputKey);
			PlayerMissedCheckpoint = PlayerRaceState.DistanceAlongSpline > LastCheckpointDistanceAlongSpline;
		}

		//Update HUD
		auto VehicleUI = PC->VehicleUI;
		check(VehicleUI);
		VehicleUI->SetPlayerMissedCheckpoint(PlayerMissedCheckpoint);
	}
}

void UAA_TimeTrialActivity::StartActivity()
{
	//Ensure Index is reset
	LastCheckpointHitIndex = -1;

	//Set First Checkpoint Active
	Track->CheckpointComponent->SpawnedCheckpoints[0]->SetActive(true);
	IndexActiveCheckpoint = 0;

	// Need immediate to update the race timer in the UI
	RegisterRewindable(ERestoreTiming::Immediate);
	
	//Start Countdown
	Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0))->VehicleUI->StartCountdown();
	FTimerHandle TimerHandle;
	//TODO don't hardcode countdown time
	GetWorld()->GetTimerManager().SetTimer(TimerHandle,this,&UAA_TimeTrialActivity::CountdownEnded,3.f,false);
}

void UAA_TimeTrialActivity::CountdownEnded()
{
	//Start Race

	//Release Player Vehicle
	auto PlayerVehicle = Cast<AAA_WheeledVehiclePawn>(UGameplayStatics::GetPlayerPawn(GetWorld(),0));
	PlayerVehicle->GetVehicleMovementComponent()->SetParked(false);

	//Start Timer
	StartTime = UGameplayStatics::GetTimeSeconds(this);
	Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0))->VehicleUI->StartTimer();
	UpdatePlayerLapsUI();

	GetWorld()->GetTimerManager().SetTimer(RaceHUDUpdateTimer, this, &UAA_TimeTrialActivity::UpdatePlayerHUD, RaceHUDUpdateFrequency, true);
}

void UAA_TimeTrialActivity::CheckpointHit(int IndexCheckpointHit, AAA_WheeledVehiclePawn* HitVehicle)
{
	if(auto PC = Cast<AAA_PlayerController>(HitVehicle->GetController()))
	{
		if(IndexCheckpointHit == LastCheckpointHitIndex + 1)
		{
			Track->CheckpointComponent->SpawnedCheckpoints[IndexCheckpointHit]->SetActive(false);
			LastCheckpointHitIndex = IndexCheckpointHit;
			UE_LOG(LogTemp,Log,TEXT("Hit Next Checkpoint Success"))
			if(LastCheckpointHitIndex == NumCheckpoints-1)
			{
				PlayerLapCounter++;
				if(PlayerLapCounter == Track->LapsToComplete)
				{
					FinishTime = UGameplayStatics::GetTimeSeconds(this);
					//UGameplayStatics::GetGameInstance(this)->StopRecordingReplay();
					FTimerHandle TimerHandle;
					GetWorld()->GetTimerManager().SetTimer(TimerHandle,this,&UAA_TimeTrialActivity::RaceEnded,FinishDelay,false);
					UE_LOG(LogTemp,Log,TEXT("Last Checkpoint Hit: Race Over"))
					GetWorld()->GetTimerManager().ClearTimer(RaceHUDUpdateTimer);
				}else
				{
					Track->CheckpointComponent->SpawnedCheckpoints[0]->SetActive(true);
					IndexActiveCheckpoint = 0;

					UpdatePlayerLapsUI();
				}
			}else
			{
				Track->CheckpointComponent->SpawnedCheckpoints[IndexCheckpointHit+1]->SetActive(true);
				IndexActiveCheckpoint = IndexCheckpointHit + 1;
			}
		}else
		{
			UE_LOG(LogTemp,Log,TEXT("Hit Next Checkpoint Out of order"))
		}
	}
}

AA_TimeTrialActivity::FSnapshotData UAA_TimeTrialActivity::CaptureSnapshot() const
{
	return AA_TimeTrialActivity::FSnapshotData
	{
		.PlayerLapCounter = PlayerLapCounter,
		.LastCheckpointHitIndex = LastCheckpointHitIndex,
		.StartTime = StartTime,
		.FinishTime = FinishTime,
		.IndexActiveCheckpoint = IndexActiveCheckpoint
	};
}

void UAA_TimeTrialActivity::RestoreFromSnapshot(const AA_TimeTrialActivity::FSnapshotData& InSnapshotData, float InRewindTime)
{
	PlayerLapCounter = InSnapshotData.PlayerLapCounter;
	LastCheckpointHitIndex = InSnapshotData.LastCheckpointHitIndex;

	Track->CheckpointComponent->SpawnedCheckpoints[IndexActiveCheckpoint]->SetActive(false);
	IndexActiveCheckpoint = InSnapshotData.IndexActiveCheckpoint;
	Track->CheckpointComponent->SpawnedCheckpoints[IndexActiveCheckpoint]->SetActive(true);

	// Offset start time by rewind time so that finish times are accurate
	StartTime = InSnapshotData.StartTime + InRewindTime;
	// If we went back after finishing race then this would reset it back to original value
	FinishTime = InSnapshotData.FinishTime;

	if (auto PC = Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)); PC)
	{
		if (auto VehicleUI = PC->VehicleUI; VehicleUI)
		{
			VehicleUI->UpdateTimerDuringRewind(InRewindTime);
			UpdatePlayerLapsUI();
		}
	}
}

void UAA_TimeTrialActivity::RaceEnded()
{
	//Play Replay
	//UGameplayStatics::GetGameInstance(this)->PlayReplay(FString("Replay"),GetWorld());
	GetWorld()->GetTimerManager().ClearTimer(RaceHUDUpdateTimer);

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle,this,&UAA_TimeTrialActivity::ReplayStartDelayEnded,ReplayStartDelay,false);
}
void UAA_TimeTrialActivity::ReplayStartDelayEnded()
{
	HideRaceUIElements();
	
	//Calculate Player Time
	UE_LOG(LogTemp,Log,TEXT("Finish Time: %f"),(FinishTime-StartTime));
	float PlayerTime = FinishTime-StartTime;

	//Park Player
	auto PlayerVehicle = Cast<AAA_WheeledVehiclePawn>(UGameplayStatics::GetPlayerPawn(GetWorld(),0));
	PlayerVehicle->GetVehicleMovementComponent()->SetParked(true);
	
	//Show Score Screen
	auto PC = Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0));
	UAA_TimeTrialScoreScreenUI* ScoreScreen = Cast<UAA_TimeTrialScoreScreenUI>(PC->BaseUI->PushMenu(ScoreScreenClass));
	bScoreScreenActive = true;

	//Add results to score screen
	TArray<FDriverName*> DriverNames;
	DriverTable->GetAllRows("", DriverNames);
	float Time =Track->FirstPlaceFinishTime;
	bool PlayerAdded = false;
	for(int i = 0; i < 10; i++)
	{
		Time += FMath::RandRange(.1f,1.f);
		if(!PlayerAdded &&Time > PlayerTime)
		{
			ScoreScreen->AddDriverScore("Player!",PlayerTime,true);
			PlayerAdded = true;
		}
		int NameIndex = FMath::RandRange(0,DriverNames.Num()-1);
		ScoreScreen->AddDriverScore(DriverNames[NameIndex]->DriverName,Time);
		DriverNames.RemoveAt(NameIndex); // don't reuse name
	}
	if(!PlayerAdded)
	{
		ScoreScreen->AddDriverScore("Player!",PlayerTime,true);
	}
}

void UAA_TimeTrialActivity::UpdatePlayerLapsUI()
{
	if (!Track)
	{
		return;
	}

	auto PC = Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	if (!PC)
	{
		return;
	}

	auto VehicleUI = PC->VehicleUI;

	if (!VehicleUI)
	{
		return;
	}

	const auto LapNum = FMath::Min(PlayerLapCounter + 1, Track->LapsToComplete);
	VehicleUI->UpdateLaps(LapNum, Track->LapsToComplete);
}

void UAA_TimeTrialActivity::HideRaceUIElements()
{
	GetWorld()->GetTimerManager().ClearTimer(RaceHUDUpdateTimer);
	bScoreScreenActive = false;

	if (auto PC = Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)); PC)
	{
		if (auto VehicleUI = PC->VehicleUI; VehicleUI)
		{
			VehicleUI->HideLaps();

			//Hide Time
			VehicleUI->HideTimer();
		}
	}
}

void UAA_TimeTrialActivity::DestroyActivity()
{
	//Unbind to Checkpoints
	for (const auto Checkpoint : Track->CheckpointComponent->SpawnedCheckpoints)
	{
		Checkpoint->CheckpointHit.RemoveDynamic(this, &UAA_TimeTrialActivity::CheckpointHit);
	}
	
	//Unload the race
	Track->UnloadRace();

	if (auto PlayerController = Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));  PlayerController)
	{
		// remove track info actor from player controller
		PlayerController->SetTrackInfo(nullptr);

		//Hide Timer (May be redundant, but we might leave before the race is over)
		if (auto VehicleUI = PlayerController->VehicleUI; VehicleUI)
		{
			VehicleUI->HideTimer();
			VehicleUI->SetPlayerMissedCheckpoint(false);
		}
	}

	GetWorld()->GetSubsystem<UAA_ActivityManagerSubsystem>()->OnDestroyActivityCompleted.Broadcast();
	HideRaceUIElements();
	UnregisterRewindable();
}

UWorld* UAA_TimeTrialActivity::GetWorld() const
{
	if(!Track)
	{
		UE_LOG(LogTemp,Warning,TEXT("%hs: No Track: GetWorld likely to fail"),__func__)
	}
	return Track->GetWorld();
}
