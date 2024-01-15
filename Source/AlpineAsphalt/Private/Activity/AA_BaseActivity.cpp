#include "Activity/AA_BaseActivity.h"

#include "Kismet/GameplayStatics.h"
#include "Activity/AA_BaseActivity.h"
#include "Controllers/AA_PlayerController.h"

void UAA_BaseActivity::Initialize(AAA_TrackInfoActor* TrackInfo)
{
}

void UAA_BaseActivity::LoadActivity()
{
}

void UAA_BaseActivity::StartActivity()
{
}

void UAA_BaseActivity::DestroyActivity()
{
}

bool UAA_BaseActivity::IsPlayerCompleted() const
{
	return false;
}

FAA_RaceState UAA_BaseActivity::GetPlayerRaceState() const
{
	auto PC = GetPlayerController();
	if (!PC)
	{
		return {};
	}
	const auto& PlayerSplineInfo = PC->GetPlayerSplineInfo();
	if (!PlayerSplineInfo)
	{
		return {};
	}
	
	return PlayerSplineInfo->RaceState;
}

AAA_PlayerController* UAA_BaseActivity::GetPlayerController() const
{
	return Cast<AAA_PlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
}

UAA_VehicleUI* UAA_BaseActivity::GetVehicleUI() const
{
	auto PC = GetPlayerController();
	if (!PC)
	{
		return nullptr;
	}

	return PC->VehicleUI;
}
