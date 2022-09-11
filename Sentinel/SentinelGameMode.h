// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "SentinelGameMode.generated.h"

namespace MatchState
{
	extern SENTINEL_API const FName Cooldown; // Match duration has been reached. Display winner and begin cooldown timer.
}
/**
 * 
 */
UCLASS()
class SENTINEL_API ASentinelGameMode : public AGameMode
{
	GENERATED_BODY()
public:
	ASentinelGameMode();
	virtual void Tick(float DeltaTime) override;
	virtual void PlayerEliminated(class ASentinelCharacter* ElimmedCharacter, class ASentinelPlayerController* VictimController, ASentinelPlayerController* AttackerController);
	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);
	void PlayerLeftGame(class ASentinelPlayerState* PlayerLeaving);

	UPROPERTY(EditDefaultsOnly)
		float WarmupTime = 10.f;

	UPROPERTY(EditDefaultsOnly)
		float MatchTime = 120.f;

	UPROPERTY(EditDefaultsOnly)
		float CooldownTime = 10.f;

	float LevelStartingTime = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;

private:
	float CountdownTime = 0.f;
public:
	FORCEINLINE float GetCountdownTime() const { return CountdownTime; }
};
