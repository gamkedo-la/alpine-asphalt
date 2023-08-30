// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AA_RaceSplineActor.generated.h"

UCLASS()
class ALPINEASPHALT_API AAA_RaceSplineActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAA_RaceSplineActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(BlueprintReadOnly,VisibleAnywhere)
	class USplineComponent* Spline;

	UPROPERTY(BlueprintReadOnly,VisibleAnywhere)
	class UAA_CheckpointComponent* CheckpointComponent;
};
