#include "Activity/AA_TimeTrialActivity.h"

#include "Actors/AA_Checkpoint.h"
#include "Actors/AA_TrackInfoActor.h"
#include "Components/AA_CheckpointComponent.h"
#include "Controllers/AA_PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Pawn/AA_WheeledVehiclePawn.h"
#include "Subsystems/AA_ActivityManagerSubsystem.h"
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
	UE_LOG(LogTemp,Warning,TEXT("Location: %s,%s,%s"),*Location.ToString(),*Track->StartLocations[0].GetLocation().ToString(),*Track->GetActorLocation().ToString());
	PlayerVehicle->SetActorTransform(FTransform(Rotation,Location),false,nullptr,ETeleportType::ResetPhysics);
	PlayerVehicle->ResetVehicle();

	//TODO: Lock Car in Place
	
	GetWorld()->GetSubsystem<UAA_ActivityManagerSubsystem>()->OnLoadActivityCompleted.Broadcast();

}

void UAA_TimeTrialActivity::CountdownEnded()
{
	//TODO: Start Race

	//Start Timer
	StartTime = UGameplayStatics::GetTimeSeconds(this);
	Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0))->VehicleUI->StartTimer();

}

void UAA_TimeTrialActivity::StartActivity()
{
	//Start Replay
	GetWorld()->Exec(GetWorld(),TEXT("demo.minrecordhz 60"));
	UGameplayStatics::GetGameInstance(this)->StartRecordingReplay(FString("Replay"),FString("Replay"));
	
	//Start Countdown
	Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(),0))->VehicleUI->StartCountdown();
	FTimerHandle TimerHandle;
	//TODO don't hardcode countdown time
	GetWorld()->GetTimerManager().SetTimer(TimerHandle,this,&UAA_TimeTrialActivity::CountdownEnded,3.f,false);



	//Wait for end of race
}

void UAA_TimeTrialActivity::DestroyActivity()
{
	//Unload the race
	Track->UnloadRace();

	//Put Player back where race started
	auto PlayerVehicle = Cast<AAA_WheeledVehiclePawn>(UGameplayStatics::GetPlayerPawn(GetWorld(),0));
	auto Rotation = Track->GetActorRotation();
	auto Location = Track->GetActorLocation();
	PlayerVehicle->SetActorTransform(FTransform(Rotation,Location),false,nullptr,ETeleportType::ResetPhysics);
	PlayerVehicle->ResetVehicle();
	
	GetWorld()->GetSubsystem<UAA_ActivityManagerSubsystem>()->OnDestroyActivityCompleted.Broadcast();
}

void UAA_TimeTrialActivity::RaceEnded()
{
	//Show Score Screen
	UE_LOG(LogTemp,Log,TEXT("Finish Time: %f"),(FinishTime-StartTime));

	GetWorld()->GetSubsystem<UAA_ActivityManagerSubsystem>()->EndActivity();

	//Play Replay
	//UGameplayStatics::GetGameInstance(this)->PlayReplay(FString("Replay"));

	//Wait for Confirmation before DestroyActivity
}

UWorld* UAA_TimeTrialActivity::GetWorld() const
{
	if(!Track)
	{
		UE_LOG(LogTemp,Warning,TEXT("%hs: No Track: GetWorld likely to fail"),__func__)
	}
	return Track->GetWorld();
}

void UAA_TimeTrialActivity::CheckpointHit(int IndexCheckpointHit)
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
			GetWorld()->GetTimerManager().SetTimer(TimerHandle,this,&UAA_TimeTrialActivity::RaceEnded,FinishDelay,false);
			UE_LOG(LogTemp,Log,TEXT("Last Checkpoint Hit: Race Over"))
		}
	}else
	{
		UE_LOG(LogTemp,Log,TEXT("Hit Next Checkpoint Out of order"))
	}
}
