// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/MainMenu.h"

#include "Components/Button.h"
#include "Components/WidgetSwitcher.h"

DEFINE_LOG_CATEGORY(LogMenu)

bool UMainMenu::Initialize()
{
	const bool InitSuccess = Super::Initialize();

	if (!InitSuccess)
	{
		UE_LOG(LogMenu, Error, TEXT("MainMenu failed to Initialize"))
		return false;
	}

	// Make sure all our widgets exist.
	if (!ensure(WidgetSwitcher != nullptr)) return false;
	if (!ensure(SettingsWidget != nullptr)) return false;
	if (!ensure(CreditsWidget != nullptr)) return false;

	// Set up click handlers for top-level buttons.
	if (!ensure(PlayButton != nullptr)) return false;
	PlayButton->OnClicked.AddDynamic(this, &UMainMenu::Play);

	if (!ensure(SettingsButton != nullptr)) return false;
	SettingsButton->OnClicked.AddDynamic(this, &UMainMenu::ShowSettings);
	
	if (!ensure(CreditsButton != nullptr)) return false;
	CreditsButton->OnClicked.AddDynamic(this, &UMainMenu::ShowCredits);
	
	if (!ensure(QuitButton != nullptr)) return false;
	QuitButton->OnClicked.AddDynamic(this, &UMainMenu::Quit);

	return true;
}

void UMainMenu::Play()
{
	auto World = GetWorld();
	if (!ensure(World != nullptr))
	{
		UE_LOG(LogMenu, Error, TEXT("MainMenu failed to get World reference"))
		return;
	}
	
	World->ServerTravel("/Game/VehicleTemplate/Maps/SmallLevelTest");
}

void UMainMenu::ShowSettings()
{
	WidgetSwitcher->SetActiveWidgetIndex(1);
	// WidgetSwitcher->SetActiveWidget(SettingsWidget);
}

void UMainMenu::ShowCredits()
{
	WidgetSwitcher->SetActiveWidgetIndex(2);
	// WidgetSwitcher->SetActiveWidget(CreditsWidget);
}

void UMainMenu::Quit()
{
	auto World = GetWorld();
	if (!ensure(World != nullptr))
	{
		UE_LOG(LogMenu, Error, TEXT("MainMenu failed to get World reference"))
		return;
	}

	GetOwningPlayer()->ConsoleCommand("quit");
}
