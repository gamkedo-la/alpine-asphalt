#include "Activity/AA_HeadToHeadActivity.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Activity/AA_TimeTrialActivity.h"
#include "Actors/AA_Checkpoint.h"
#include "Actors/AA_TrackInfoActor.h"
#include "Algo/RandomShuffle.h"
#include "Components/AA_CheckpointComponent.h"
#include "Controllers/AA_AIRacerController.h"
#include "Controllers/AA_PlayerController.h"
#include "Race/AA_RaceState.h"
#include "Kismet/GameplayStatics.h"
#include "Pawn/AA_WheeledVehiclePawn.h"
#include "Subsystems/AA_ActivityManagerSubsystem.h"
#include "UI/AA_BaseUI.h"
#include "UI/AA_TimeTrialScoreScreenUI.h"
#include "UI/AA_VehicleUI.h"

void UAA_HeadToHeadActivity::Initialize(AAA_TrackInfoActor* TrackToUse)
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

void UAA_HeadToHeadActivity::LoadActivity()
{
	//Start Replay
	UGameplayStatics::GetGameInstance(this)->StartRecordingReplay(FString("Replay"),FString("Replay"));
	
	//Spawn DataLayers, Despawn DataLayers, Spawn Checkpoints
	Track->LoadRace();

	int LastStartingPosition = Track->StartLocations.Num() - 1;
	
	//Move Player Vehicle to Start Position
	auto PlayerVehicle = Cast<AAA_WheeledVehiclePawn>(UGameplayStatics::GetPlayerPawn(GetWorld(),0));
	check(PlayerVehicle);

	FRotator Rotation = Track->StartLocations[LastStartingPosition].Rotator() + Track->GetActorRotation();
	FVector Location = Track->GetActorRotation().RotateVector(Track->StartLocations[LastStartingPosition].GetLocation());
	Location += Track->GetActorLocation();
	PlayerVehicle->SetActorTransform(FTransform(Rotation,Location),false,nullptr,ETeleportType::ResetPhysics);
	PlayerVehicle->ResetVehicle();
	LapsCompletedMap.Reset();
	LapsCompletedMap.Add(PlayerVehicle,0);

	auto PlayerController = Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	check(PlayerController);
	PlayerController->SetTrackInfo(Track);

	AAA_AIRacerController* AI;
	AAA_WheeledVehiclePawn* AIVehicle;
	int RandomIndex;
	
	//Spawn an AI for every other Starting Position and Park
	for(int i = 0; i < Track->StartLocations.Num() - 1; i++)
	{
		Rotation = Track->StartLocations[i].Rotator() + Track->GetActorRotation();
		Location = Track->GetActorRotation().RotateVector(Track->StartLocations[i].GetLocation());
		Location += Track->GetActorLocation();
		AIVehicle = GetWorld()->SpawnActor<AAA_WheeledVehiclePawn>(VehicleClass,Location,Rotation);
		if(AIVehicle)
		{
			AIVehicle->SetColorOne(FColor( //Random RGB
									FMath::RandRange(0,256),
									FMath::RandRange(0,256),
									FMath::RandRange(0,256)));
			RandomIndex = FMath::RandRange(0,VehiclesToSpawn.Num()-1);
			AIVehicle->SetVehicleData(VehiclesToSpawn[RandomIndex]);
			AIVehicle->GetVehicleMovementComponent()->SetParked(true);
			AIVehicle->SpawnDefaultController();
			AI = Cast<AAA_AIRacerController>(AIVehicle->Controller);
			if(AI)
			{
				AI->SetTrackInfo(Track);
			}
			AIRacers.Add(AIVehicle);
			//AI need to hit the finish line twice in a circuit because they start behind the finish line
			if(Track->IsCircuit)
			{
				LapsCompletedMap.Add(AIVehicle,0);
			}else
			{
				LapsCompletedMap.Add(AIVehicle,1);
			}
		}
	}
	//Park
	PlayerVehicle->GetVehicleMovementComponent()->SetParked(true);

	//Load Driver Names
	//Get Driver Names
	TArray<FDriverName*> DriverNamesOut;
	DriverTable->GetAllRows("", DriverNamesOut);
	for (const auto NamesOut : DriverNamesOut)
	{
		DriverNames.Add(*NamesOut);
	}
	
	//Listen to Checkpoints
	for (const auto Checkpoint : Track->CheckpointComponent->SpawnedCheckpoints)
	{
		Checkpoint->CheckpointHit.AddDynamic(this, &UAA_HeadToHeadActivity::CheckpointHit);
	}
	
	GetWorld()->Exec(GetWorld(),TEXT("demo.MinRecordHz 60"));
	GetWorld()->Exec(GetWorld(),TEXT("demo.RecordHz 120"));

	GetWorld()->GetSubsystem<UAA_ActivityManagerSubsystem>()->OnLoadActivityCompleted.Broadcast();
}

void UAA_HeadToHeadActivity::StartActivity()
{
	//Ensure Index is reset
	LastCheckpointHitIndex = -1;
	
	//Start Countdown
	Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0))->VehicleUI->StartCountdown();
	FTimerHandle TimerHandle;
	//TODO don't hardcode countdown time
	GetWorld()->GetTimerManager().SetTimer(TimerHandle,this,&UAA_HeadToHeadActivity::CountdownEnded,3.f,false);
}

void UAA_HeadToHeadActivity::CountdownEnded()
{
	//Start Race

	//Release Vehicles
	auto PlayerVehicle = Cast<AAA_WheeledVehiclePawn>(UGameplayStatics::GetPlayerPawn(GetWorld(),0));
	PlayerVehicle->GetVehicleMovementComponent()->SetParked(false);

	for(int i = 0; i < AIRacers.Num(); i++)
	{
		AIRacers[i]->GetVehicleMovementComponent()->SetParked(false);
	}

	//Start Timer
	StartTime = UGameplayStatics::GetTimeSeconds(this);
	Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0))->VehicleUI->StartTimer();

	RegisterRacePositionTimer();
}

void UAA_HeadToHeadActivity::CheckpointHit(int IndexCheckpointHit, AAA_WheeledVehiclePawn* HitVehicle)
{
	
	// Show race finish on map and mini-map after hitting second checkpoint
	if (IndexCheckpointHit >= 1)
	{
		Track->CheckpointComponent->ShowRaceFinish(true);
	}

	if(auto PC = Cast<AAA_PlayerController>(HitVehicle->GetController()))
	{
		if(IndexCheckpointHit == LastCheckpointHitIndex + 1)
		{
			LastCheckpointHitIndex = IndexCheckpointHit;
			UE_LOG(LogTemp,Log,TEXT("Hit Next Checkpoint Success"))
			if(LastCheckpointHitIndex == NumCheckpoints-1) // Player finished race
			{
				LapsCompletedMap[HitVehicle]++;
				if(LapsCompletedMap[HitVehicle] == Track->LapsToComplete)
				{
					FinishTime = UGameplayStatics::GetTimeSeconds(this);
					UGameplayStatics::GetGameInstance(this)->StopRecordingReplay();
					FTimerHandle TimerHandle;
					GetWorld()->GetTimerManager().SetTimer(TimerHandle,this,&UAA_HeadToHeadActivity::RaceEnded,FinishDelay,false);
					UE_LOG(LogTemp, Log, TEXT("Last Checkpoint Hit: Race Over"))
						//TODO: Add to Score Screen

					UnRegisterRacePositionTimer();
				}else
				{
					//TODO: increment Laps display
				}
			}
		}else
		{
			UE_LOG(LogTemp,Log,TEXT("Hit Next Checkpoint Out of order"))
		}
	}else
	{
		if(IndexCheckpointHit == NumCheckpoints-1) // Non-Player finished race
		{
			LapsCompletedMap[HitVehicle]++;
			//AI need to hit the finish line twice for one lap because we assume they start behind it
			if(LapsCompletedMap[HitVehicle] == Track->LapsToComplete+1) // AI Finished Race
			{
				float TimeToFinish = UGameplayStatics::GetTimeSeconds(this)-StartTime;
				if(ScoreScreen) //if UI already exists add the score, otherwise save it for later in array
				{
					AddAIDriverScore(TimeToFinish);
				}else
				{
					FinishTimes.Add(TimeToFinish);
				}
			}
		}
	}
}

void UAA_HeadToHeadActivity::AddAIDriverScore(float TimeScore)
{
	
	int NameIndex = FMath::RandRange(0,DriverNames.Num()-1);
	ScoreScreen->AddDriverScore(DriverNames[NameIndex].DriverName,TimeScore);
	DriverNames.RemoveAt(NameIndex); // dont reuse name
}

void UAA_HeadToHeadActivity::HideRaceUIElements(AAA_PlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	auto VehicleUI = PlayerController->VehicleUI;
	if (!VehicleUI)
	{
		return;
	}

	//Hide Time
	VehicleUI->HideTimer();

	UnRegisterRacePositionTimer();
	VehicleUI->HideRacePosition();

	//TODO: Hide Laps
}

void UAA_HeadToHeadActivity::RegisterRacePositionTimer()
{
	auto World = GetWorld();
	check(World);

	World->GetTimerManager().SetTimer(RacePositionUpdateTimer, this, &UAA_HeadToHeadActivity::UpdateRacePosition, RacePositionUpdateFrequency, true);
}

void UAA_HeadToHeadActivity::UnRegisterRacePositionTimer()
{
	auto World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(RacePositionUpdateTimer);
}

void UAA_HeadToHeadActivity::UpdateRacePosition()
{
	// Add Player state
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
	const auto PlayerCompletionFraction = PlayerRaceState.GetOverallCompletionFraction();

	int32 RacerCount { 1 };
	int32 RacePosition { 1 };

	// Compare to AI Racer States
	for (auto Racer : AIRacers)
	{
		if (!Racer)
		{
			continue;
		}

		auto RacerController = Cast<AAA_AIRacerController>(Racer->GetController());
		if (!RacerController)
		{
			continue;
		}

		const auto& AIRacerState = RacerController->GetRacerContext().RaceState;
		++RacerCount;

		if (AIRacerState.GetOverallCompletionFraction() > PlayerCompletionFraction)
		{
			++RacePosition;
		}
	}

	auto VehicleUI = PC->VehicleUI;
	check(VehicleUI);

	VehicleUI->UpdateRacePosition(RacePosition, RacerCount);
}

void UAA_HeadToHeadActivity::RaceEnded()
{
	//Play Replay
	//UGameplayStatics::GetGameInstance(this)->PlayReplay(FString("Replay"),GetWorld());

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle,this,&UAA_HeadToHeadActivity::ReplayStartDelayEnded,ReplayStartDelay,false);

	UnRegisterRacePositionTimer();
}

void UAA_HeadToHeadActivity::ReplayStartDelayEnded()
{
	//Calculate Player Time
	UE_LOG(LogTemp,Log,TEXT("Finish Time: %f"),(FinishTime-StartTime));
	float PlayerTime = FinishTime-StartTime;

	//Show Score Screen
	auto PC = Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0));
	ScoreScreen = Cast<UAA_TimeTrialScoreScreenUI>(PC->BaseUI->PushMenu(ScoreScreenClass));

	bool PlayerTimeAdded = false;
	
	for(int i = 0; i < AIRacers.Num(); i++)
	{
		if(i < FinishTimes.Num()) //if there are finish times to display
		{
			if(!PlayerTimeAdded && PlayerTime < FinishTimes[i]) //if the player should be added first
			{
				PlayerTimeAdded = true;
				ScoreScreen->AddDriverScore("Player!",PlayerTime,true);
			}
			AddAIDriverScore(FinishTimes[i]);
		}
	}
	if(!PlayerTimeAdded)//if the player was last to finish
	{
		ScoreScreen->AddDriverScore("Player!",PlayerTime,true);
	}
	
	HideRaceUIElements(PC);
}
void UAA_HeadToHeadActivity::DestroyActivity()
{
	//Unbind to Checkpoints
	for (const auto Checkpoint : Track->CheckpointComponent->SpawnedCheckpoints)
	{
		Checkpoint->CheckpointHit.RemoveDynamic(this, &UAA_HeadToHeadActivity::CheckpointHit);
	}
	
	//Unload the race
	Track->UnloadRace();

	//Put Player back where race started
	auto PlayerVehicle = Cast<AAA_WheeledVehiclePawn>(UGameplayStatics::GetPlayerPawn(GetWorld(),0));
	auto Rotation = Track->GetActorRotation();
	auto Location = Track->GetActorLocation();
	PlayerVehicle->SetActorTransform(FTransform(Rotation,Location),false,nullptr,ETeleportType::ResetPhysics);
	PlayerVehicle->ResetVehicle();

	//Hide Timer and other UI elements specific to the race (May be redundant, but we might leave before the race is over)
	HideRaceUIElements(Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0)));

	ScoreScreen = nullptr;

	GetWorld()->GetSubsystem<UAA_ActivityManagerSubsystem>()->OnDestroyActivityCompleted.Broadcast();
}

UWorld* UAA_HeadToHeadActivity::GetWorld() const
{
	if(!Track)
	{
		UE_LOG(LogTemp,Warning,TEXT("%hs: No Track: GetWorld likely to fail"),__func__)
	}
	return Track->GetWorld();
}
