// Fill out your copyright notice in the Description page of Project Settings.


#include "SentinelPlayerController.h"
#include "Sentinel/HUD/CharacterOverlay.h"
#include "Sentinel/HUD/SentinelHUD.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Sentinel/Character/SentinelCharacter.h"
#include "Net/UnrealNetwork.h"
#include "Sentinel/SentinelGameMode.h"
#include "Sentinel/PlayerState/SentinelPlayerState.h"
#include "Sentinel/HUD/Announcement.h"
#include "Kismet/GameplayStatics.h"
#include "Sentinel/SentinelComponents/CombatComponent.h"
#include "Sentinel/GameState/SentinelGameState.h"
#include "Components/Image.h"
#include "Sentinel/HUD/ReturnToMainMennu.h"

void ASentinelPlayerController::BroadcastElim(APlayerState* Attacker, APlayerState* Victim)
{
	ClientElimAnnouncement(Attacker, Victim);
}

void ASentinelPlayerController::ClientElimAnnouncement_Implementation(APlayerState* Attacker, APlayerState* Victim)
{
	APlayerState* Self = GetPlayerState<APlayerState>();
	if (Attacker && Victim && Self)
	{
		SentinelHUD = SentinelHUD == nullptr ? Cast<ASentinelHUD>(GetHUD()) : SentinelHUD;
		if (SentinelHUD)
		{
			if (Attacker == Self && Victim != Self)
			{
				SentinelHUD->AddElimAnnouncement("You", Victim->GetPlayerName());
				return;
			}
			if (Victim == Self && Attacker != Self)
			{
				SentinelHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "You");
				return;
			}
			if (Attacker == Victim && Attacker == Self)
			{
				SentinelHUD->AddElimAnnouncement("You", "yourself");
				return;
			}
			if (Attacker == Victim && Attacker != Self)
			{
				SentinelHUD->AddElimAnnouncement(Attacker->GetPlayerName(), "themselves");
				return;
			}
			SentinelHUD->AddElimAnnouncement(Attacker->GetPlayerName(), Victim->GetPlayerName());

		}
	}
}

void ASentinelPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SentinelHUD = Cast<ASentinelHUD>(GetHUD());
	ServerCheckMatchState();

	
}

void ASentinelPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASentinelPlayerController, MatchState);
}

void ASentinelPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SetHUDTime();
	CheckTimeSync(DeltaTime);
	PollInit();
	CheckPing(DeltaTime);
}

void ASentinelPlayerController::CheckPing(float DeltaTime)
{
	HighPingRunningTime += DeltaTime;
	if (HighPingRunningTime > CheckPingFrequency)
	{
		PlayerState = PlayerState == nullptr ? GetPlayerState<APlayerState>() : PlayerState;
		if (PlayerState)
		{
			if (PlayerState->GetPing() * 4 > HighPingThreshold) // ping is compressed; it's actually ping / 4
			{
				HighPingWarning();
				PingAnimationRunningTime = 0.f;
				ServerReportPingStatus(true);
			}
			else
			{
				ServerReportPingStatus(false);
			}
		}
		HighPingRunningTime = 0.f;
	}
	bool bHighPingAnimationPlaying =
		SentinelHUD && SentinelHUD->CharacterOverlay &&
		SentinelHUD->CharacterOverlay->HighPingAnimation &&
		SentinelHUD->CharacterOverlay->IsAnimationPlaying(SentinelHUD->CharacterOverlay->HighPingAnimation);
	if (bHighPingAnimationPlaying)
	{
		PingAnimationRunningTime += DeltaTime;
		if (PingAnimationRunningTime > HighPingDuration)
		{
			StopHighPingWarning();
		}
	}
}

void ASentinelPlayerController::ShowReturnToMainMenu()
{
	if (ReturnToMainMenu == nullptr) return;
	if (ReturnToMainMenu == nullptr)
	{
		ReturnToMainMenu = CreateWidget<UReturnToMainMennu>(this, ReturnToMainMenuWidget);
	}
	if (ReturnToMainMenu)
	{
		bReturnToMainMenu = !bReturnToMainMenu;
		if (ReturnToMainMenu)
		{
			ReturnToMainMenu->MenuSetup();
		}
		else
		{
			ReturnToMainMenu->MenuTearDown();
		}
	}
}

// Is ping too high??
void ASentinelPlayerController::ServerReportPingStatus_Implementation(bool bHighPing)
{
	HighPingDelegate.Broadcast(bHighPing);
}

void ASentinelPlayerController::CheckTimeSync(float DeltaTime)
{
	TimeSyncRunningTime += DeltaTime;
	if (IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
	
}

void ASentinelPlayerController::HighPingWarning()
{
	SentinelHUD = SentinelHUD == nullptr ? Cast<ASentinelHUD>(GetHUD()) : SentinelHUD;
	bool bHUDValid = SentinelHUD &&
		SentinelHUD->CharacterOverlay &&
		SentinelHUD->CharacterOverlay->HighPingImage &&
		SentinelHUD->CharacterOverlay->HighPingAnimation;
	if (bHUDValid)
	{
		SentinelHUD->CharacterOverlay->HighPingImage->SetOpacity(1.f);
		SentinelHUD->CharacterOverlay->PlayAnimation(
			SentinelHUD->CharacterOverlay->HighPingAnimation,
			0.f,
			5);
	}
}

void ASentinelPlayerController::StopHighPingWarning()
{
	SentinelHUD = SentinelHUD == nullptr ? Cast<ASentinelHUD>(GetHUD()) : SentinelHUD;
	bool bHUDValid = SentinelHUD &&
		SentinelHUD->CharacterOverlay &&
		SentinelHUD->CharacterOverlay->HighPingImage &&
		SentinelHUD->CharacterOverlay->HighPingAnimation;
	if (bHUDValid)
	{
		SentinelHUD->CharacterOverlay->HighPingImage->SetOpacity(0.f);
		if (SentinelHUD->CharacterOverlay->IsAnimationPlaying(SentinelHUD->CharacterOverlay->HighPingAnimation))
		{
			SentinelHUD->CharacterOverlay->StopAnimation(SentinelHUD->CharacterOverlay->HighPingAnimation);
		}
	}
}

void ASentinelPlayerController::ServerCheckMatchState_Implementation()
{
	ASentinelGameMode* GameMode = Cast<ASentinelGameMode>(UGameplayStatics::GetGameMode(this));
	if (GameMode)
	{
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		LevelStartingTime = GameMode->LevelStartingTime;
		MatchState = GameMode->GetMatchState();
		ClientJoinMidGame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartingTime);

		if (SentinelHUD && MatchState == MatchState::WaitingToStart)
		{
			SentinelHUD->AddAnnouncement();
		}
	}
}


void ASentinelPlayerController::ClientJoinMidGame_Implementation(FName StateOfMatch, float Warmup, float Match, float Cooldown, float StartingTime)
{
	WarmupTime = Warmup;
	MatchTime = MatchTime;
	CooldownTime = Cooldown;
	LevelStartingTime = StartingTime;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);
	if (SentinelHUD && MatchState == MatchState::WaitingToStart)
	{
		SentinelHUD->AddAnnouncement();
	}
}

void ASentinelPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ASentinelCharacter* SentinelCharacter = Cast<ASentinelCharacter>(InPawn);
	if (SentinelCharacter)
	{
		SetHUDHealth(SentinelCharacter->GetHealth(), SentinelCharacter->GetMaxHealth());
	}
}

void ASentinelPlayerController::SetHUDHealth(float Health, float MaxHealth)
{
	SentinelHUD = SentinelHUD == nullptr ? Cast<ASentinelHUD>(GetHUD()) : SentinelHUD;
	bool bHUDValid = SentinelHUD && SentinelHUD->CharacterOverlay && SentinelHUD->CharacterOverlay->HealthBar && SentinelHUD->CharacterOverlay->HealthText;
	if (bHUDValid)
	{
		const float HealthPercent = Health / MaxHealth;
		SentinelHUD->CharacterOverlay->HealthBar->SetPercent(HealthPercent);
		FString HealthText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Health), FMath::CeilToInt(MaxHealth));
		SentinelHUD->CharacterOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitializeHealth = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ASentinelPlayerController::SetHUDShield(float Shield, float MaxShield)
{
	SentinelHUD = SentinelHUD == nullptr ? Cast<ASentinelHUD>(GetHUD()) : SentinelHUD;
	bool bHUDValid = SentinelHUD && SentinelHUD->CharacterOverlay && SentinelHUD->CharacterOverlay->ShieldBar && SentinelHUD->CharacterOverlay->ShieldText;
	if (bHUDValid)
	{
		const float ShieldPercent = Shield / MaxShield;
		SentinelHUD->CharacterOverlay->ShieldBar->SetPercent(ShieldPercent);
		FString ShieldText = FString::Printf(TEXT("%d/%d"), FMath::CeilToInt(Shield), FMath::CeilToInt(MaxShield));
		SentinelHUD->CharacterOverlay->ShieldText->SetText(FText::FromString(ShieldText));
	}
	else
	{
		bInitializeShield = true;
		HUDShield = Shield;
		HUDMaxShield = MaxShield;
	}
}

void ASentinelPlayerController::SetHUDScore(float Score)
{
	SentinelHUD = SentinelHUD == nullptr ? Cast<ASentinelHUD>(GetHUD()) : SentinelHUD;
	bool bHUDValid = SentinelHUD && SentinelHUD->CharacterOverlay && SentinelHUD->CharacterOverlay->ScoreAmount;
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		SentinelHUD->CharacterOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializeScore = true;
		HUDScore = Score;
	}
}

void ASentinelPlayerController::SetHUDDefeats(int32 Defeats)
{
	SentinelHUD = SentinelHUD == nullptr ? Cast<ASentinelHUD>(GetHUD()) : SentinelHUD;
	bool bHUDValid = SentinelHUD && SentinelHUD->CharacterOverlay && SentinelHUD->CharacterOverlay->DefeatsAmount;
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		SentinelHUD->CharacterOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializeDefeats = true;
		HUDDefeats = Defeats;
	}
}

void ASentinelPlayerController::SetHUDWeaponAmmo(int32 Ammo)
{
	SentinelHUD = SentinelHUD == nullptr ? Cast<ASentinelHUD>(GetHUD()) : SentinelHUD;
	bool bHUDValid = SentinelHUD && SentinelHUD->CharacterOverlay && SentinelHUD->CharacterOverlay->WeaponAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		SentinelHUD->CharacterOverlay->WeaponAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeWeaponAmmo = true;
		HUDWeaponAmmo = Ammo;
	}
}

void ASentinelPlayerController::SetHUDCarriedAmmo(int32 Ammo)
{
	SentinelHUD = SentinelHUD == nullptr ? Cast<ASentinelHUD>(GetHUD()) : SentinelHUD;
	bool bHUDValid = SentinelHUD && SentinelHUD->CharacterOverlay && SentinelHUD->CharacterOverlay->CarriedAmmoAmount;
	if (bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		SentinelHUD->CharacterOverlay->CarriedAmmoAmount->SetText(FText::FromString(AmmoText));
	}
	else
	{
		bInitializeCarriedAmmo = true;
		HUDCarriedAmmo = Ammo;
	}
}

void ASentinelPlayerController::SetHUDMatchCountdown(float CountdownTime)
{
	SentinelHUD = SentinelHUD == nullptr ? Cast<ASentinelHUD>(GetHUD()) : SentinelHUD;
	bool bHUDValid = SentinelHUD && SentinelHUD->CharacterOverlay && SentinelHUD->CharacterOverlay->MatchCountdownText;
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			SentinelHUD->CharacterOverlay->MatchCountdownText->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		SentinelHUD->CharacterOverlay->MatchCountdownText->SetText(FText::FromString(CountdownText));
	}
}

void ASentinelPlayerController::SetHUDGrenades(int32 Grenades)
{
	SentinelHUD = SentinelHUD == nullptr ? Cast<ASentinelHUD>(GetHUD()) : SentinelHUD;
	bool bHUDValid = SentinelHUD && SentinelHUD->CharacterOverlay && SentinelHUD->CharacterOverlay->GrenadesText;
	if (bHUDValid)
	{
		FString GrenadesText = FString::Printf(TEXT("%d"), Grenades);
		SentinelHUD->CharacterOverlay->GrenadesText->SetText(FText::FromString(GrenadesText));
	}
	else
	{
		bInitializeGrenades = true;
		HUDGrenades = Grenades;
	}
}

void ASentinelPlayerController::SetHUDAnnouncementCountdown(float CountdownTime)
{
	SentinelHUD = SentinelHUD == nullptr ? Cast<ASentinelHUD>(GetHUD()) : SentinelHUD;
	bool bHUDValid = SentinelHUD && SentinelHUD->Announcement && SentinelHUD->Announcement->WarmupTime;
	if (bHUDValid)
	{
		if (CountdownTime < 0.f)
		{
			SentinelHUD->Announcement->WarmupTime->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(CountdownTime / 60.f);
		int32 Seconds = CountdownTime - Minutes * 60;

		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		SentinelHUD->Announcement->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void ASentinelPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent == nullptr) return;

	InputComponent->BindAction("Quit", IE_Pressed, this, &ASentinelPlayerController::ShowReturnToMainMenu);


}

void ASentinelPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if (MatchState == MatchState::WaitingToStart) TimeLeft = WarmupTime - GetServerTime() + LevelStartingTime;
	else if(MatchState == MatchState::InProgress) TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	else if(MatchState == MatchState::Cooldown) TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartingTime;
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	if (HasAuthority())
	{
		SentinelGameMode = SentinelGameMode == nullptr ? Cast<ASentinelGameMode>(UGameplayStatics::GetGameMode(this)) : SentinelGameMode;
		if (SentinelGameMode)
		{
			SecondsLeft = FMath::CeilToInt(SentinelGameMode->GetCountdownTime() + LevelStartingTime);
		}
	}

	if (CountdownInt != SecondsLeft)
	{
		if (MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDAnnouncementCountdown(TimeLeft);
		}
		if (MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountdown(TimeLeft);
		}
	}
	CountdownInt = SecondsLeft;
}

void ASentinelPlayerController::PollInit()
{
	if (CharacterOverlay == nullptr)
	{
		if (SentinelHUD && SentinelHUD->CharacterOverlay)
		{
			CharacterOverlay = SentinelHUD->CharacterOverlay;
			if (CharacterOverlay)
			{
				if(bInitializeHealth) SetHUDHealth(HUDHealth, HUDMaxHealth);
				if (bInitializeShield) SetHUDShield(HUDShield, HUDMaxShield);
				if (bInitializeScore) SetHUDScore(HUDScore);
				if (bInitializeDefeats) SetHUDDefeats(HUDDefeats);
				if (bInitializeCarriedAmmo) SetHUDCarriedAmmo(HUDCarriedAmmo);
				if (bInitializeWeaponAmmo) SetHUDWeaponAmmo(HUDWeaponAmmo);

				ASentinelCharacter* SentinelCharacter = Cast<ASentinelCharacter>(GetPawn());
				if (SentinelCharacter && SentinelCharacter->GetCombat())
				{
					if (bInitializeGrenades) SetHUDGrenades(SentinelCharacter->GetCombat()->GetGrenades());
				}
			}
		}
	}
}

void ASentinelPlayerController::ServerRequestServerTime_Implementation(float TimeOfClientRequest)
{
	float ServerTimeOfReceipt = GetWorld()->GetTimeSeconds();
	ClientReportServerTime(TimeOfClientRequest, ServerTimeOfReceipt);
}

void ASentinelPlayerController::ClientReportServerTime_Implementation(float TimeOfClientRequest, float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeOfClientRequest;
	SingleTripTime = 0.5f * RoundTripTime;
	float CurrentServerTime = TimeServerReceivedClientRequest + SingleTripTime;
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}


float ASentinelPlayerController::GetServerTime()
{
	if (HasAuthority()) return GetWorld()->GetTimeSeconds();
	return GetWorld()->GetTimeSeconds() + ClientServerDelta;
}

void ASentinelPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	if (IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ASentinelPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ASentinelPlayerController::OnRep_MatchState()
{
	if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ASentinelPlayerController::HandleMatchHasStarted()
{
	SentinelHUD = SentinelHUD == nullptr ? Cast<ASentinelHUD>(GetHUD()) : SentinelHUD;
	if (SentinelHUD)
	{
		if(SentinelHUD->CharacterOverlay == nullptr)
		SentinelHUD->AddCharacterOverlay();
		if (SentinelHUD->Announcement)
		{
			SentinelHUD->Announcement->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ASentinelPlayerController::HandleCooldown()
{
	SentinelHUD = SentinelHUD == nullptr ? Cast<ASentinelHUD>(GetHUD()) : SentinelHUD;
	if (SentinelHUD)
	{
		SentinelHUD->CharacterOverlay->RemoveFromParent();
		bool bHUDValid = SentinelHUD->Announcement && SentinelHUD->Announcement->AnnouncementText && SentinelHUD->Announcement->InfoText;
		if (bHUDValid)
		{
			SentinelHUD->Announcement->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText("New Match Starts in:");
			SentinelHUD->Announcement->AnnouncementText->SetText(FText::FromString(AnnouncementText));

			ASentinelGameState* SentinelGameState = Cast <ASentinelGameState>(UGameplayStatics::GetGameState(this));
			ASentinelPlayerState* SentinelPlayerState = GetPlayerState<ASentinelPlayerState>();
			if (SentinelGameState && SentinelPlayerState)
			{
				TArray<ASentinelPlayerState*> TopPlayers = SentinelGameState->TopScoringPlayers;
				FString InfoTextString;
				if (TopPlayers.Num() == 0)
				{
					InfoTextString = FString("There is no winner.");
				}
				else if (TopPlayers.Num() == 1 && TopPlayers[0] == SentinelPlayerState)
				{
					InfoTextString = FString("You are the winner!");
				}
				else if (TopPlayers.Num() == 1)
				{
					InfoTextString = FString::Printf(TEXT("Winner: \n%s"), *TopPlayers[0]->GetPlayerName());
				}
				else if (TopPlayers.Num() > 1)
				{
					InfoTextString = FString("Players tied for the win:\n");
					for (auto TiedPlayer : TopPlayers)
					{
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
					}
				}
				SentinelHUD->Announcement->InfoText->SetText(FText::FromString(InfoTextString));
			}
		}
	}
	ASentinelCharacter* SentinelCharacter = Cast<ASentinelCharacter>(GetPawn());
	if (SentinelCharacter && SentinelCharacter->GetCombat())
	{
		SentinelCharacter->bDisableGameplay = true;
		SentinelCharacter->GetCombat()->FireButtonPressed(false);
	}
}
