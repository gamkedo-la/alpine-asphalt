// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/AA_InteractableInterface.h"
#include "AA_TrackInfoActor.generated.h"

UCLASS()
class ALPINEASPHALT_API AAA_TrackInfoActor : public AActor, public IAA_InteractableInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAA_TrackInfoActor();
	
	UPROPERTY(BlueprintReadOnly,VisibleAnywhere, Category=Race)
	class USplineComponent* Spline;

	UPROPERTY(BlueprintReadOnly,EditAnywhere)
	TArray<float> RoadWidth;
	
	UPROPERTY(BlueprintReadOnly,VisibleAnywhere, Category=Race)
	class UAA_CheckpointComponent* CheckpointComponent;

	UPROPERTY(BlueprintReadOnly,VisibleAnywhere, Category=Race)
	class USphereComponent* InteractableCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Race, Meta = (MakeEditWidget = true))
	TArray<FTransform> StartLocations;
	
	UPROPERTY(BlueprintReadWrite, EditInstanceOnly, Category=Race)
	TArray<UDataLayerAsset*> DataLayersToLoad;

	UPROPERTY(BlueprintReadWrite, EditInstanceOnly, Category=Race)
	TArray<UDataLayerAsset*> DataLayersToUnload;
	
	UFUNCTION()
	float GetWidthAtDistance(float Distance);
	
	UFUNCTION(CallInEditor, Category=Race)
	void LoadRace();

	UFUNCTION(CallInEditor, Category=Race)
	void UnloadRace();

	UFUNCTION()
	void OnOverlapBegin(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(class UPrimitiveComponent* OverlappedComp, class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	UFUNCTION(BlueprintCallable)
	virtual void Interact(AAA_PlayerController* Interactor) override;
};
