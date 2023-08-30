// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AA_CheckpointComponent.generated.h"


class USplineComponent;

USTRUCT(BlueprintType)
struct FCheckpointStruct
{
	GENERATED_BODY()
	FCheckpointStruct(): Position(0), Width(0)
	{
	}
	FCheckpointStruct(const float Position): Width(0)
	{
		this->Position = Position;
	}
	FCheckpointStruct(const float Position, const float Width)
	{
		this->Position = Position;
		this->Width = Width;
	}
	UPROPERTY(BlueprintReadWrite,EditAnywhere)
	float Position;
	UPROPERTY(BlueprintReadWrite,EditAnywhere)
	float Width;
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
	
	UPROPERTY(BlueprintReadWrite,EditInstanceOnly,Category=Checkpoints)
	float CheckpointGenerationDistance = 5000.f;

	UPROPERTY(BlueprintReadWrite,EditInstanceOnly,Category=Checkpoints)
	TArray<FCheckpointStruct> CheckpointPositionData;

private:
	UPROPERTY()
	USplineComponent* SplineComponent;
};
