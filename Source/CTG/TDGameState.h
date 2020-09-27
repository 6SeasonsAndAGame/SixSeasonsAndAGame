// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "TDGameState.generated.h"

/**
 * 
 */
UCLASS()
class CTG_API ATDGameStateBase : public AGameState
{
	GENERATED_BODY()

public:

	ATDGameStateBase();

	UPROPERTY(EditAnywhere, Category = Teams)
		uint8 NumTeams;

	UPROPERTY(EditAnywhere, Category = Teams)
		uint32 MaxTeamScore;

	UPROPERTY(Transient)
	TArray<uint8> TeamScores;
};
