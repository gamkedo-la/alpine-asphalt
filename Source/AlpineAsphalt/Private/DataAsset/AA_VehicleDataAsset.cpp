// Fill out your copyright notice in the Description page of Project Settings.


#include "DataAsset/AA_VehicleDataAsset.h"

#if WITH_EDITOR
	void UAA_VehicleDataAsset ::PostEditChangeProperty( FPropertyChangedEvent & PropertyChangedEvent )
	{
		Super::PostEditChangeProperty( PropertyChangedEvent );
		OnValueChanged.Broadcast();
	}
#endif