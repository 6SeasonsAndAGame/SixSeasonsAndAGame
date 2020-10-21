// Fill out your copyright notice in the Description page of Project Settings.

#include "FPSPlayerController.h"

AFPSPlayerController::AFPSPlayerController() :
	Super() {

}

void AFPSPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
}


void AFPSPlayerController::CreateUI_Implementation() 
{
	OnCreateUI();
}

void AFPSPlayerController::UpdateUI_Implementation() 
{
	OnUpdateUI();
}