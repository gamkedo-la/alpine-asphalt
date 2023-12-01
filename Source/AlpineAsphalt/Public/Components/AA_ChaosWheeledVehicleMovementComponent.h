#pragma once
#include "ChaosWheeledVehicleMovementComponent.h"
#include "AA_ChaosWheeledVehicleMovementComponent.generated.h"

UCLASS(ClassGroup = (Physics), meta = (BlueprintSpawnableComponent), hidecategories = (PlanarMovement, "Components|Movement|Planar", Activation, "Components|Activation"))
class ALPINEASPHALT_API UAA_ChaosWheeledVehicleMovementComponent : public UChaosWheeledVehicleMovementComponent
{

GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category=ChaosWheel)
	void SetWheelCorneringStiffness(int WheelIndex,float NewCorneringStiffness);

	UFUNCTION(BlueprintCallable, Category=ChaosWheel)
	float GetWheelCorneringStiffness(int WheelIndex);
	
};
