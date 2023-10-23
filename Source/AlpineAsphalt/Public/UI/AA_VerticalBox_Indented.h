// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/VerticalBox.h"
#include "AA_VerticalBox_Indented.generated.h"

/**
 * 
 */
UCLASS()
class ALPINEASPHALT_API UAA_VerticalBox_Indented : public UVerticalBox
{
	GENERATED_BODY()

private:
	void SetIndentForAllChildren();
protected:
	
	virtual void OnSlotAdded(UPanelSlot* InSlot) override;
	virtual void OnSlotRemoved(UPanelSlot* InSlot) override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;


public:

	UPROPERTY(EditAnywhere)
	float IndentAmount = 50.f;

	UPROPERTY(EditAnywhere)
	bool ReverseIndentOrder = false;
	
};
