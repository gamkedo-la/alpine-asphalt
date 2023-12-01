
#include "Components/AA_ChaosWheeledVehicleMovementComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"

void UAA_ChaosWheeledVehicleMovementComponent::SetWheelCorneringStiffness(int WheelIndex, float NewCorneringStiffness)
{
 if (FBodyInstance* TargetInstance = GetBodyInstance())
 {
  FPhysicsCommand::ExecuteWrite(TargetInstance->ActorHandle, [&](const FPhysicsActorHandle& Chassis)
  {
   if (VehicleSimulationPT && VehicleSimulationPT->PVehicle && WheelIndex < VehicleSimulationPT->PVehicle->Wheels.Num())
   {
    Chaos::FSimpleWheelSim& VehicleWheel = VehicleSimulationPT->PVehicle->Wheels[WheelIndex];

    VehicleWheel.CorneringStiffness = NewCorneringStiffness;
   }
  });
 }
}

float UAA_ChaosWheeledVehicleMovementComponent::GetWheelCorneringStiffness(int WheelIndex)
{
 if (VehicleSimulationPT && VehicleSimulationPT->PVehicle && WheelIndex < VehicleSimulationPT->PVehicle->Wheels.Num())
 {
  return VehicleSimulationPT->PVehicle->Wheels[WheelIndex].CorneringStiffness;
 }
 return 0.f;
}
