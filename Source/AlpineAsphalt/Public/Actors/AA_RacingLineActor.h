// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AA_RacingLineActor.generated.h"

UCLASS()
class ALPINEASPHALT_API AAA_RacingLineActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AAA_RacingLineActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable)
	void SetArrowMeshes();
	
protected:

	UPROPERTY(BlueprintReadOnly,EditAnywhere)
	class USplineComponent* RacingLineSpline;

	UPROPERTY(BlueprintReadOnly,EditAnywhere)
	class UInstancedStaticMeshComponent* InstancedArrowMeshes;

	UPROPERTY(BlueprintReadWrite,EditDefaultsOnly)
	UStaticMesh* ArrowMesh;

	UPROPERTY(EditAnywhere)
	float DistanceBetweenArrows = 300.f;

	
};
