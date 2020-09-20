// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "TDPlayerStart.generated.h"

/**
 * 
 */
UCLASS()
class CTG_API ATDPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:
	UPROPERTY(EditInstanceOnly, Category = Team)
		uint8 Team;

	UPROPERTY(EditInstanceOnly, Category = Team)
		bool bCanPlayerSpawn;

	UPROPERTY(EditInstanceOnly, Category = Team)
		bool bCanBotSpawn;
};
