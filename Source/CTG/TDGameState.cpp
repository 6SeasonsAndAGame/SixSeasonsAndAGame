// Fill out your copyright notice in the Description page of Project Settings.


#include "TDGameState.h"

ATDGameStateBase::ATDGameStateBase(): Super() {
	NumTeams = 2;
	MaxTeamScore = 25;
	TeamScores.AddDefaulted(NumTeams);
}