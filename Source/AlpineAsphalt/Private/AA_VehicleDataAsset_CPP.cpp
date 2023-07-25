// Fill out your copyright notice in the Description page of Project Settings.


#include "AA_VehicleDataAsset_CPP.h"

#if WITH_EDITOR
	void UAA_VehicleDataAsset_CPP ::PostEditChangeProperty( FPropertyChangedEvent & PropertyChangedEvent )
	{
		Super::PostEditChangeProperty( PropertyChangedEvent );
		OnValueChanged.Broadcast();
	}
#endif