// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AA_MapCaptureActor.generated.h"

class UMapCaptureComponent;

UCLASS()
class ALPINEASPHALT_API AAA_MapCaptureActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AAA_MapCaptureActor();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = Map)
	TObjectPtr<UMapCaptureComponent> MapCaptureComponent;
};
