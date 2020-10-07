// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "TDPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class CTG_API ATDPlayerState : public APlayerState
{
	GENERATED_BODY()


public:
	ATDPlayerState();

	uint32 GetPlayerElims() { return PlayerElims; }
	uint32 GetTimesEliminated() { return TimesEliminated; }

	void OnEliminated() { TimesEliminated++; }
	void OnPlayerEliminate() { PlayerElims++; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int Team;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int TimesEliminated = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int PlayerElims = 0;
};
