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
	void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const;

	uint32 GetPlayerElims() { return PlayerElims; }
	uint32 GetTimesEliminated() { return TimesEliminated; }

	void OnEliminated() { TimesEliminated++; }
	void OnPlayerEliminate() { PlayerElims++; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	int Team;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Transient)
	int TimesEliminated;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Transient)
	int PlayerElims;
};
