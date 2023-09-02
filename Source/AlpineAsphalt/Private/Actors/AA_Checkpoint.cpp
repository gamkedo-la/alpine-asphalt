#include "Actors/AA_Checkpoint.h"

#include "Components/BoxComponent.h"
#include "Controllers/AA_PlayerController.h"
#include "Pawn/AA_WheeledVehiclePawn.h"

AAA_Checkpoint::AAA_Checkpoint()
{
	Collision = CreateDefaultSubobject<UBoxComponent>("Collision");
	Collision->OnComponentBeginOverlap.AddDynamic(this,&AAA_Checkpoint::OnOverlapBegin);
}

void AAA_Checkpoint::SetHeight(const float Height) const
{
	FVector Extent = Collision->Bounds.BoxExtent;
	Extent.Z = Height;
	Collision->SetBoxExtent(Extent);
}

void AAA_Checkpoint::SetWidth(const float Width) const
{
	FVector Extent = Collision->Bounds.BoxExtent;
	Extent.Y = Width;
	Collision->SetBoxExtent(Extent);
}

void AAA_Checkpoint::SetIndex(const int NewIndex)
{
	Index = NewIndex;
}

void AAA_Checkpoint::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if(auto HitVehicle = Cast<AAA_WheeledVehiclePawn>(OtherActor))
	{
		if(auto PC = Cast<AAA_PlayerController>(HitVehicle->GetController()))
		{
			CheckpointHit.Broadcast(Index);
		}
	}
}
