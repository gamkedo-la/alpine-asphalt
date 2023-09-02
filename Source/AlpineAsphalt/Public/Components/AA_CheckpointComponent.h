// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AA_CheckpointComponent.generated.h"


class AAA_Checkpoint;
class USplineComponent;

#define DEFAULT_CHECKPOINT_HEIGHT 500

USTRUCT(BlueprintType)
struct FCheckpointStruct
{
	GENERATED_BODY()
	FCheckpointStruct(): Position(0), Width(0), Height(0){
		
	}

	FCheckpointStruct(const FVector& Position, const FRotator& Rotation) : Width(0), Height(0)
	{
		this->Position = Position;
		this->Rotation = Rotation;
	}

	FCheckpointStruct(const FVector& Position, const FRotator& Rotation, const float Width, const float Height)
	{
		this->Position = Position;
		this->Width = Width;
		this->Height = Height;
	}
	
	//Position of Checkpoint
	UPROPERTY(BlueprintReadWrite,EditAnywhere)
	FVector Position;

	UPROPERTY()
	FRotator Rotation;

	//Width of Checkpoint
	UPROPERTY(BlueprintReadWrite,EditAnywhere)
	float Width;

	//Height of Checkpoint
	UPROPERTY(BlueprintReadWrite,EditAnywhere)
	float Height;
	
};


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALPINEASPHALT_API UAA_CheckpointComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAA_CheckpointComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	virtual void InitializeComponent() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(CallInEditor,BlueprintCallable, Category=Checkpoints)
	void GenerateCheckpoints();

	UFUNCTION()
	USplineComponent* GetSpline() const;

	UFUNCTION(BlueprintCallable)
	void SpawnCheckpoints();

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	TSubclassOf<AAA_Checkpoint> DefaultCheckpoint;

	UPROPERTY()
	TArray<AAA_Checkpoint*> SpawnedCheckpoints; 
	
	UPROPERTY(BlueprintReadWrite,EditInstanceOnly,Category=Checkpoints)
	float CheckpointGenerationDistance = 5000.f;

	UPROPERTY(BlueprintReadWrite,EditInstanceOnly,Category=Checkpoints)
	TArray<FCheckpointStruct> CheckpointPositionData;





private:
	UPROPERTY()
	USplineComponent* SplineComponent;
};
