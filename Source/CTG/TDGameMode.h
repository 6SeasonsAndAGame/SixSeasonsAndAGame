// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TDGameMode.generated.h"

/**
 * 
 */
UCLASS(config = game)
class CTG_API ATDGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	ATDGameMode();

	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual void PostLogin(APlayerController* PlayerController) override;
	/** new player joins */
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

	virtual void BeginPlay() override;

	void OnPlayerEliminated(class AController* Eliminator, class AController* EliminatedPlayer);
	void OnTeamWin(uint8 WinningTeam);
	bool HasTeamWon(uint8 Team);
	
	void DefaultTimer();
};
