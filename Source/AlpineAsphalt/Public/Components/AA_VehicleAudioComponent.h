// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AA_VehicleAudioComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ALPINEASPHALT_API UAA_VehicleAudioComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UAA_VehicleAudioComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAudioComponent>EngineAudioComponent;

	void UpdateEnginePitch() const;


public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	static FName VehicleAudioComponentName;
	UPROPERTY(EditAnywhere, Category="Engine Pitch")
	float MinEnginePitch = 0.5f;

	UPROPERTY(EditAnywhere, Category="Engine Pitch")
	float MidEnginePitch = 1.0f;

	UPROPERTY(EditAnywhere, Category="Engine Pitch")
	float MaxEnginePitch = 1.5f;
		
};
