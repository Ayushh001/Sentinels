// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LagCompensationComponent.generated.h"


USTRUCT(BlueprintType)
struct FBoxInformation
{
	GENERATED_BODY()

	UPROPERTY()
	FVector Location;

	UPROPERTY()
	FRotator Rotation;

	UPROPERTY()
	FVector BoxExtent;
};

USTRUCT(BlueprintType)
struct FFramePackage
{
	GENERATED_BODY()

	UPROPERTY()
	float Time;

	UPROPERTY()
	TMap<FName, FBoxInformation> HitBoxInfo;

	UPROPERTY()
	ASentinelCharacter* Character;
};

USTRUCT(BlueprintType)
struct FServerSideRewindResult
{
	GENERATED_BODY()

	UPROPERTY()
	bool bHitConfirmed;

	UPROPERTY()
	bool bHeadShot;
};

USTRUCT(BlueprintType)
struct FShotgunServerSideRewindResult
{
	GENERATED_BODY()

		UPROPERTY()
		TMap<ASentinelCharacter*, uint32> HeadShots;
		
		UPROPERTY()
		TMap<ASentinelCharacter*, uint32> BodyShots;
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SENTINEL_API ULagCompensationComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	ULagCompensationComponent();
	friend class ASentinelCharacter;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**
	* Hitscan
	*/

	FServerSideRewindResult ServerSideRewind(
		class ASentinelCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime);

	/**
	* Projectile
	*/

	FServerSideRewindResult ProjectileServerSideRewind(
		ASentinelCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime
	);
	/**
	* Shotgun
	*/

	FShotgunServerSideRewindResult ShotgunServerSideRewind(
		const TArray<ASentinelCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		float HitTime);


	UFUNCTION(Server, Reliable)
	void ServerScoreRequest(
		class ASentinelCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation,
		float HitTime
		);
		
	UFUNCTION(Server, Reliable)
		void ProjectileServerScoreRequest(
			ASentinelCharacter* HitCharacter,
			const FVector_NetQuantize& TraceStart,
			const FVector_NetQuantize100& InitialVelocity,
			float HitTime
		);

	UFUNCTION(Server, Reliable)
	void ShotgunServerScoreRequest(
		const TArray<ASentinelCharacter*>& HitCharacters,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations,
		float HitTime
		);

protected:
	virtual void BeginPlay() override;
	void SaveFramePackage(FFramePackage& Package);
	FFramePackage InterpBetweenFrames(const FFramePackage& OlderFrame, const FFramePackage& YoungerFrame, float HitTime);
	void CacheBoxPositions(ASentinelCharacter* HitCharacter, FFramePackage& OutFramePackage);
	void MoveBoxes(ASentinelCharacter* HitCharacter, const FFramePackage& Package);
	void ResetHitBoxes(ASentinelCharacter* HitCharacter, const FFramePackage& Package);
	void EnableCharacterMeshCollision(ASentinelCharacter* HitCharacter, ECollisionEnabled::Type CollisionEnabled);
	void SaveFramePackage();
	FFramePackage GetFrameToCheck(ASentinelCharacter* HitCharacter, float HitTime);


	/**
	* Hitscan
	*/

	FServerSideRewindResult ConfirmedHit(
		const FFramePackage& Package,
		ASentinelCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize& HitLocation);

	/**
	* Projectile
	*/
	FServerSideRewindResult ProjectileConfirmedHit(
		const FFramePackage& Package,
		ASentinelCharacter* HitCharacter,
		const FVector_NetQuantize& TraceStart,
		const FVector_NetQuantize100& InitialVelocity,
		float HitTime
	);

	/*
	Shotgun
	*/

	FShotgunServerSideRewindResult ShotgunConfirmedHit(
		const TArray<FFramePackage>& FramePackages,
		const FVector_NetQuantize& TraceStart,
		const TArray<FVector_NetQuantize>& HitLocations);

private:

	UPROPERTY()
	ASentinelCharacter* Character;

	UPROPERTY()
	class ASentinelPlayerController* Controller;
	
	TDoubleLinkedList<FFramePackage> FrameHistory;

	UPROPERTY(EditAnywhere)
	float MaxRecordTime = 4.f;
};
