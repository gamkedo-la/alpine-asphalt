// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/AA_RaceActor.h"
#include "Components/AA_CheckpointComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "Controllers/AA_PlayerController.h"
#include "GameFramework/GameSession.h"
#include "Pawn/AA_WheeledVehiclePawn.h"
#include "WorldPartition/DataLayer/DataLayerSubsystem.h"

// Sets default values
AAA_RaceActor::AAA_RaceActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	Spline = CreateDefaultSubobject<USplineComponent>("RacingLineSpline");
	CheckpointComponent = CreateDefaultSubobject<UAA_CheckpointComponent>("CheckpointComponent");
	InteractableCollision = CreateDefaultSubobject<USphereComponent>("InteractableCollision");
	SetRootComponent(Spline);

	InteractableCollision->OnComponentBeginOverlap.AddDynamic(this, &AAA_RaceActor::OnOverlapBegin);
	InteractableCollision->OnComponentEndOverlap.AddDynamic(this,&AAA_RaceActor::OnOverlapEnd);
	InteractableCollision->SetupAttachment(Spline);
	InteractableCollision->InitSphereRadius(500.f);
}

// Called when the game starts or when spawned
void AAA_RaceActor::BeginPlay()
{
	Super::BeginPlay();
}

float AAA_RaceActor::GetWidthAtDistance(float Distance)
{
	float Key = Spline->GetInputKeyAtDistanceAlongSpline(Distance);
	int Lower = FMath::FloorToInt(Key);
	int Upper = FMath::CeilToInt(Key);
	if(Lower < 0)
	{
		if(RoadWidth.IsEmpty()){ return 0; }
		return RoadWidth[0];			
	}
	if(Upper > RoadWidth.Num())
	{
		return RoadWidth.Last();
	}
	const float Alpha = FMath::Fractional(Key);
	return FMath::Lerp(RoadWidth[Lower],RoadWidth[Upper],Alpha);
}

void AAA_RaceActor::LoadRace()
{
	if(HasAuthority())
	{
		if(UDataLayerSubsystem* DataLayerSubsystem = UWorld::GetSubsystem<UDataLayerSubsystem>(GetWorld()))
		{
			for (const auto DataLayer : DataLayersToLoad)
			{
				DataLayerSubsystem->SetDataLayerInstanceRuntimeState(DataLayer,EDataLayerRuntimeState::Activated);
			}
			for (const auto DataLayer : DataLayersToUnload)
			{
				DataLayerSubsystem->SetDataLayerInstanceRuntimeState(DataLayer,EDataLayerRuntimeState::Unloaded);
			}
		}
	}
	//Spawn Checkpoints
	
}

void AAA_RaceActor::UnloadRace()
{
	if(HasAuthority())
	{
		if(UDataLayerSubsystem* DataLayerSubsystem = UWorld::GetSubsystem<UDataLayerSubsystem>(GetWorld()))
		{
			for (const auto DataLayer : DataLayersToUnload)
			{
				DataLayerSubsystem->SetDataLayerInstanceRuntimeState(DataLayer,EDataLayerRuntimeState::Activated);
			}
			for (const auto DataLayer : DataLayersToLoad)
			{
				DataLayerSubsystem->SetDataLayerInstanceRuntimeState(DataLayer,EDataLayerRuntimeState::Unloaded);
			}
		}
	}
}

void AAA_RaceActor::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(const auto HitVehicle = Cast<AAA_WheeledVehiclePawn>(OtherActor))
	{
		if(const auto PC = Cast<AAA_PlayerController>(HitVehicle->Controller))
		{
			PC->AddInteractables(this);
			UE_LOG(LogTemp,Warning,TEXT("Hit Race"));
		}
	}
}

void AAA_RaceActor::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if(const auto HitVehicle = Cast<AAA_WheeledVehiclePawn>(OtherActor))
	{
		if(const auto PC = Cast<AAA_PlayerController>(HitVehicle->Controller))
		{
			PC->RemoveInteractable(this);
			UE_LOG(LogTemp,Warning,TEXT("Left Race"));
		}
	}
}

void AAA_RaceActor::Interact(AAA_PlayerController* Interactor)
{
	//Set Actor at start
	Interactor->GetPawn()->SetActorLocation(Spline->GetComponentLocation());
	//Load the Race Decorations
	LoadRace();
	//Spawn CheckPoints
	CheckpointComponent->SpawnCheckpoints();
}


