#include "Activity/AA_HeadToHeadActivity.h"

#include "ChaosWheeledVehicleMovementComponent.h"
#include "Actors/AA_Checkpoint.h"
#include "Actors/AA_TrackInfoActor.h"
#include "Components/AA_CheckpointComponent.h"
#include "Controllers/AA_AIRacerController.h"
#include "Controllers/AA_PlayerController.h"
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

	//Listen to Checkpoints
	for (const auto Checkpoint : Track->CheckpointComponent->SpawnedCheckpoints)
	{
		Checkpoint->CheckpointHit.AddDynamic(this, &UAA_HeadToHeadActivity::CheckpointHit);
	}

	int LastStartingPosition = Track->StartLocations.Num() - 1;
	
	//Move Player Vehicle to Start Position
	auto PlayerVehicle = Cast<AAA_WheeledVehiclePawn>(UGameplayStatics::GetPlayerPawn(GetWorld(),0));
	FRotator Rotation = Track->StartLocations[LastStartingPosition].Rotator() + Track->GetActorRotation();
	FVector Location = Track->GetActorRotation().RotateVector(Track->StartLocations[LastStartingPosition].GetLocation());
	Location += Track->GetActorLocation();
	PlayerVehicle->SetActorTransform(FTransform(Rotation,Location),false,nullptr,ETeleportType::ResetPhysics);
	PlayerVehicle->ResetVehicle();

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
		}
	}
	
	PlayerVehicle->GetVehicleMovementComponent()->SetParked(true);
	
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

}

void UAA_HeadToHeadActivity::CheckpointHit(int IndexCheckpointHit)
{
	if(IndexCheckpointHit == LastCheckpointHitIndex + 1)
	{
		LastCheckpointHitIndex = IndexCheckpointHit;
		UE_LOG(LogTemp,Log,TEXT("Hit Next Checkpoint Success"))
		if(LastCheckpointHitIndex == NumCheckpoints-1)
		{
			FinishTime = UGameplayStatics::GetTimeSeconds(this);
			UGameplayStatics::GetGameInstance(this)->StopRecordingReplay();
			FTimerHandle TimerHandle;
			GetWorld()->GetTimerManager().SetTimer(TimerHandle,this,&UAA_HeadToHeadActivity::RaceEnded,FinishDelay,false);
			UE_LOG(LogTemp,Log,TEXT("Last Checkpoint Hit: Race Over"))
		}
	}else
	{
		UE_LOG(LogTemp,Log,TEXT("Hit Next Checkpoint Out of order"))
	}
}

void UAA_HeadToHeadActivity::RaceEnded()
{
	//Play Replay
	//UGameplayStatics::GetGameInstance(this)->PlayReplay(FString("Replay"),GetWorld());

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle,this,&UAA_HeadToHeadActivity::ReplayStartDelayEnded,ReplayStartDelay,false);
}
void UAA_HeadToHeadActivity::ReplayStartDelayEnded()
{
	//Calculate Player Time
	UE_LOG(LogTemp,Log,TEXT("Finish Time: %f"),(FinishTime-StartTime));
	float PlayerTime = FinishTime-StartTime;

	//Show Score Screen
	auto PC = Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0));
	UAA_TimeTrialScoreScreenUI* ScoreScreen = Cast<UAA_TimeTrialScoreScreenUI>(PC->BaseUI->PushMenu(ScoreScreenClass));

	//TODO: Add results to score screen
	/*
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
		ScoreScreen->AddDriverScore(DriverNames[i]->DriverName,Time);
	}
	if(!PlayerAdded)
	{
		ScoreScreen->AddDriverScore("Player!",PlayerTime,true);
	}
	*/
	
	//Hide Time
	PC->VehicleUI->HideTimer();
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

	//Hide Timer (May be redundant, but we might leave before the race is over)
	Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0))->VehicleUI->HideTimer();

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
