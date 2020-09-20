// Fill out your copyright notice in the Description page of Project Settings.


#include "TDGameMode.h"
#include "EngineUtils.h"
#include "TDPlayerState.h"
#include "FPSPlayerController.h"
#include "TDPlayerStart.h"
#include "TDGameStateBase.h"

ATDGameMode::ATDGameMode() : Super()
{

}

AActor* ATDGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	return nullptr;
}

void ATDGameMode::PostLogin(APlayerController* PlayerController)
{
	Super::PostLogin(PlayerController);

	static int LastTeam = 0;
	ATDPlayerState* PlayerState = PlayerController->GetPlayerState<ATDPlayerState>();
	PlayerState->SetTeam(LastTeam);
	UE_LOG(LogTemp, Warning, TEXT("Player joined team %i"), PlayerState->GetTeam()
	);
	LastTeam = LastTeam == 0? 1 : 0; // Bitwise AND To switch between 0 and 1

	TArray<ATDPlayerStart*> PlayerTeamStarts;

	ATDPlayerState* CurrentPlayerState = PlayerController->GetPlayerState<ATDPlayerState>();
	for (TActorIterator<ATDPlayerStart> It(GetWorld()); It; ++It)
	{
		if (CurrentPlayerState->GetTeam() == It->Team) {
			PlayerTeamStarts.Add(*It);
		}
	}
	ATDPlayerStart* FinalSpawnPoint = PlayerTeamStarts[FMath::RandRange(0, PlayerTeamStarts.Num() - 1)];

	UE_LOG(LogTemp, Warning, TEXT("Team: %i, Had %i Possible spawn locations"), PlayerState->GetTeam(), PlayerTeamStarts.Num());

	auto NewCharacter = GetWorld()->SpawnActor<APawn>(DefaultPawnClass, FinalSpawnPoint->GetActorLocation(), FinalSpawnPoint->GetActorRotation());
	PlayerController->Possess(NewCharacter);
}

void ATDGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

void ATDGameMode::BeginPlay()
{
	Super::BeginPlay();
	FTimerHandle UnusedHandle;
	GetWorldTimerManager().SetTimer(UnusedHandle, this, &ATDGameMode::DefaultTimer, GetWorldSettings()->GetEffectiveTimeDilation(), true);
}

void ATDGameMode::OnPlayerEliminated(AController* Eliminator, AController* EliminatedPlayer)
{
	auto EliminatorPS = Eliminator->GetPlayerState<ATDPlayerState>();
	auto EliminatedPS = EliminatedPlayer->GetPlayerState<ATDPlayerState>();
	if (EliminatorPS != nullptr) {
		EliminatorPS->OnPlayerEliminate();
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("EliminatorPS was nullptr"));
	}
	
	if (EliminatedPS != nullptr) {
		EliminatedPS->OnEliminated();
	} else {
		UE_LOG(LogTemp, Warning, TEXT("EliminatedPS was nullptr"));
	}

	if (EliminatorPS != nullptr) {
		GetGameState<ATDGameStateBase>()->TeamScores[Eliminator->GetPlayerState<ATDPlayerState>()->GetTeam()]++; //Increment the team's score/elims
		UE_LOG(LogTemp, Warning, TEXT("Team %i got a kill!"), Eliminator->GetPlayerState<ATDPlayerState>()->GetTeam());
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("EliminatorPS was nullptr"));
	}


	RestartPlayer(EliminatedPlayer); // Restart/Respawn PlayerController
	// Display to everyone's UI Instigator Eliminated EliminatedPlayer

	// Determine whether or not a team has now won due to the elimination & call OnTeamWin if a team has won
	if (HasTeamWon(EliminatorPS->GetTeam())) {
		OnTeamWin(EliminatorPS->GetTeam());
	}
}

void ATDGameMode::OnTeamWin(uint8 WinningTeam)
{
	// Display Team win UI to all players
	UE_LOG(LogTemp, Warning, TEXT("Team %i Won!"), WinningTeam);
	auto GS = GetGameState<ATDGameStateBase>();
	UE_LOG(LogTemp, Warning, TEXT("Team 1 Score: %i, Team 2 Score: %i"), GS->TeamScores[0], GS->TeamScores[1]);

	RestartGame();
}

bool ATDGameMode::HasTeamWon(uint8 Team)
{
	for (auto& TeamScore : GetGameState<ATDGameStateBase>()->TeamScores) {
		if (TeamScore >= GetGameState<ATDGameStateBase>()->MaxTeamScore) {
			return true;
		}
	}
	return false;
}

void ATDGameMode::DefaultTimer()
{
	FName CurrentMatchState = GetMatchState();
	if (CurrentMatchState == MatchState::WaitingToStart) {
		auto GS = GetGameState<ATDGameStateBase>();
		UE_LOG(LogTemp, Warning, TEXT("Team 1 Score: %i, Team 2 Score: %i"), GS->TeamScores[0], GS->TeamScores[1]);
		StartMatch();
	}
	else if (CurrentMatchState == MatchState::InProgress) {
		// Potentially do something?
	}
	else if (CurrentMatchState == MatchState::WaitingPostMatch) {
		RestartGame();
	}
}
