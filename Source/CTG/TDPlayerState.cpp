// Fill out your copyright notice in the Description page of Project Settings.


#include "TDPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"

ATDPlayerState::ATDPlayerState() : Super()
{
	Team = 0;
}

void ATDPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ATDPlayerState, Team);
	DOREPLIFETIME(ATDPlayerState, TimesEliminated);
	DOREPLIFETIME(ATDPlayerState, PlayerElims);
}

