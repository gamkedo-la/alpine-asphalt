#pragma once

#include "AA_Checkpoint.generated.h"

class UBoxComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCheckpointHitDelegate,int,CheckpointIndex);

UCLASS(Blueprintable,BlueprintType)
class ALPINEASPHALT_API AAA_Checkpoint : public AActor
{
	GENERATED_BODY()

public:

	AAA_Checkpoint();

	void SetHeight(float Height) const;
	void SetWidth(float Width) const;
	void SetIndex(int Index);

	UPROPERTY(BlueprintAssignable)
	FCheckpointHitDelegate CheckpointHit;
	
private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(EditAnywhere)
	UBoxComponent* Collision;

	int Index = -1;
	
};
