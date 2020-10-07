// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FPSPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class CTG_API AFPSPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	AFPSPlayerController();

	virtual void SetupInputComponent() override;
	
	UFUNCTION(BlueprintImplementableEvent)
		void CreateUI();

	UFUNCTION(BlueprintImplementableEvent)
		void UpdateUI();


};
