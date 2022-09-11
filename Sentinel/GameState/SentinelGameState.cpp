// Fill out your copyright notice in the Description page of Project Settings.


#include "SentinelGameState.h"
#include "Net/UnrealNetwork.h"
#include "Sentinel/PlayerState/SentinelPlayerState.h"

void ASentinelGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASentinelGameState, TopScoringPlayers);
	DOREPLIFETIME(ASentinelGameState, RedTeamScore);
	DOREPLIFETIME(ASentinelGameState, BlueTeamScore);
}

void ASentinelGameState::UpdateTopScore(ASentinelPlayerState* ScoringPlayer)
{
	if (TopScoringPlayers.Num() == 0)
	{
		TopScoringPlayers.Add(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
	else if (ScoringPlayer->GetScore() == TopScore)
	{
		TopScoringPlayers.AddUnique(ScoringPlayer);
	}
	else if (ScoringPlayer->GetScore() > TopScore)
	{
		TopScoringPlayers.Empty();
		TopScoringPlayers.AddUnique(ScoringPlayer);
		TopScore = ScoringPlayer->GetScore();
	}
}

void ASentinelGameState::OnRep_RedTeamScore()
{
}

void ASentinelGameState::OnRep_BlueTeamScore()
{
}
