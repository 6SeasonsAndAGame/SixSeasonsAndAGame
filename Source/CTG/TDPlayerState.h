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

	uint8 Team;
	uint32 TimesEliminated = 0;
	uint32 PlayerElims = 0;
public:
	ATDPlayerState();

	uint8 GetTeam() { return Team; }
	//uint32 GetPlayerScore() { return PlayerElims * 100; }
	uint32 GetPlayerElims() { return PlayerElims; }
	uint32 GetTimesEliminated() { return TimesEliminated; }

	void SetTeam(uint8 _Team) { Team = _Team; }
	void OnEliminated() { TimesEliminated++; }
	void OnPlayerEliminate() { PlayerElims++; }
};
