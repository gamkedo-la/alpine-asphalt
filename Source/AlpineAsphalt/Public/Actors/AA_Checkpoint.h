#pragma once
#include "Pawn/AA_WheeledVehiclePawn.h"
#include "AA_Checkpoint.generated.h"

class UBoxComponent;
class UPaperSpriteComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FCheckpointHitSignature, int,CheckpointIndex,AAA_WheeledVehiclePawn*,VehicleHit);

UCLASS(Blueprintable,BlueprintType)
class ALPINEASPHALT_API AAA_Checkpoint : public AActor
{
	GENERATED_BODY()

public:

	AAA_Checkpoint();

	void SetHeight(float Height) const;
	void SetWidth(float Width) const;
	void SetDepth(float Depth) const;
	void SetSize(FVector Size)const;
	void SetIndex(int Index);
	void SetIndicatorVisibleInMap(bool bVisible);

	UFUNCTION(BlueprintNativeEvent)
	void SetActive(bool IsActive);
	UPROPERTY(BlueprintAssignable)
	FCheckpointHitSignature CheckpointHit;
	
	UPROPERTY(BlueprintReadOnly,EditAnywhere)
	UBoxComponent* Collision;
	
private:
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UPaperSpriteComponent> MapIndicator{};

	int Index = -1;
	
};
