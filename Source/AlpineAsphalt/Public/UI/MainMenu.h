// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenu.generated.h"

/**
 * 
 */
DECLARE_LOG_CATEGORY_EXTERN(LogMenu, Log, All);
UCLASS()
class ALPINEASPHALT_API UMainMenu : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual bool Initialize() override;

	void Play();
	void ShowSettings();
	void ShowCredits();
	void Quit();

	/** Widgets **/
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UWidgetSwitcher* WidgetSwitcher;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UWidget* SettingsWidget;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UWidget* CreditsWidget;

	/** Buttons **/
	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UButton* PlayButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UButton* SettingsButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UButton* CreditsButton;

	UPROPERTY(BlueprintReadOnly, meta=(BindWidget))
	class UButton* QuitButton;

private:
	int32 ActiveWidgetIndex;
};
