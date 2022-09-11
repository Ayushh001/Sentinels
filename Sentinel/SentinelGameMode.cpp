// Fill out your copyright notice in the Description page of Project Settings.


#include "SentinelGameMode.h"
#include "Sentinel/Character/SentinelCharacter.h"
#include "Sentinel/PlayerController/SentinelPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Sentinel/PlayerState/SentinelPlayerState.h"
#include "Sentinel/GameState/SentinelGameState.h"

namespace MatchState
{
	const FName Cooldown = FName("Cooldown");
}

ASentinelGameMode::ASentinelGameMode()
{
	bDelayedStart = true;
}

void ASentinelGameMode::BeginPlay()
{
	Super::BeginPlay();

	LevelStartingTime = GetWorld()->GetTimeSeconds();
}

void ASentinelGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MatchState == MatchState::WaitingToStart)
	{
		CountdownTime = WarmupTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			StartMatch();
		}
	}
	else if (MatchState == MatchState::InProgress)
	{
		CountdownTime = WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			SetMatchState(MatchState::Cooldown);
		}
	}
	else if (MatchState == MatchState::Cooldown)
	{
		CountdownTime = CooldownTime + WarmupTime + MatchTime - GetWorld()->GetTimeSeconds() + LevelStartingTime;
		if (CountdownTime <= 0.f)
		{
			RestartGame();
		}
	}
}

void ASentinelGameMode::OnMatchStateSet()
{
	Super::OnMatchStateSet();
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ASentinelPlayerController* SentinelPlayer = Cast<ASentinelPlayerController>(*It);
		if (SentinelPlayer)
		{
			SentinelPlayer->OnMatchStateSet(MatchState);
		}
	}

}

void ASentinelGameMode::PlayerEliminated(class ASentinelCharacter* ElimmedCharacter, class ASentinelPlayerController* VictimController, ASentinelPlayerController* AttackerController)
{
	ASentinelPlayerState* AttackerPlayerState = AttackerController ? Cast<ASentinelPlayerState>(AttackerController->PlayerState) : nullptr;
	ASentinelPlayerState* VictimPlayerState = VictimController ? Cast<ASentinelPlayerState>(VictimController->PlayerState) : nullptr;
	
	ASentinelGameState* SentinelGameState = GetGameState<ASentinelGameState>();

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState && SentinelGameState)
	{
		TArray<ASentinelPlayerState*>PlayersCurrentlyInTheLead;
		for (auto LeadPlayer : SentinelGameState->TopScoringPlayers)
		{
			PlayersCurrentlyInTheLead.Add(LeadPlayer);
		}

		AttackerPlayerState->AddToScore(1.f);
		SentinelGameState->UpdateTopScore(AttackerPlayerState);
		if (SentinelGameState->TopScoringPlayers.Contains(AttackerPlayerState))
		{
			ASentinelCharacter* Leader = Cast<ASentinelCharacter>(AttackerPlayerState->GetPawn());
			if (Leader)
			{
				Leader->MulticastGainedTheLead();
			}
		}

		for (int32 i = 0; i < PlayersCurrentlyInTheLead.Num(); i++)
		{
			if(!SentinelGameState->TopScoringPlayers.Contains(PlayersCurrentlyInTheLead[i]))
			{
				ASentinelCharacter* Loser = Cast<ASentinelCharacter>(PlayersCurrentlyInTheLead[i]->GetPawn());
				if (Loser)
				{
					Loser->MulticastLostTheLead();
				}
			}
		}
	}
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Elim(false);
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		ASentinelPlayerController* SentinelPlayer = Cast<ASentinelPlayerController>(*It);
		if (SentinelPlayer && AttackerPlayerState && VictimPlayerState)
		{
			SentinelPlayer->BroadcastElim(AttackerPlayerState, VictimPlayerState);
		}
	}
}

void ASentinelGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if (ElimmedCharacter)
	{
		ElimmedCharacter->Reset();
		ElimmedCharacter->Destroy(); 
	}
	if (ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);

	}
}

void ASentinelGameMode::PlayerLeftGame(class ASentinelPlayerState* PlayerLeaving)
{
	if (PlayerLeaving == nullptr) return;
	ASentinelGameState* SentinelGameState = GetGameState<ASentinelGameState>();
	if (SentinelGameState && SentinelGameState->TopScoringPlayers.Contains(PlayerLeaving))
	{
		SentinelGameState->TopScoringPlayers.Remove(PlayerLeaving);
	}
	ASentinelCharacter* CharacterLeaving = Cast<ASentinelCharacter>(PlayerLeaving->GetPawn());
	if (CharacterLeaving)
	{
		CharacterLeaving->Elim(true);
	}
}


