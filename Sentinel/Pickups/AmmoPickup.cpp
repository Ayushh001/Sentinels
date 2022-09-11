// Fill out your copyright notice in the Description page of Project Settings.


#include "AmmoPickup.h"
#include "Sentinel/Character/SentinelCharacter.h"
#include "Sentinel/SentinelComponents/CombatComponent.h"

void AAmmoPickup::OnSphereOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult
)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ASentinelCharacter* SentinelCharacter = Cast<ASentinelCharacter>(OtherActor);
	if (SentinelCharacter)
	{
		UCombatComponent* Combat = SentinelCharacter->GetCombat();
		if (Combat)
		{
			Combat->PickupAmmo(WeaponType, AmmoAmount);
		}
	}
	Destroy();

}

