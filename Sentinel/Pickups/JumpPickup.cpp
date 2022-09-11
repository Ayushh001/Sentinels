// Fill out your copyright notice in the Description page of Project Settings.


#include "JumpPickup.h"
#include "Sentinel/Character/SentinelCharacter.h"
#include "Sentinel/SentinelComponents/BuffComponent.h"

void AJumpPickup::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	ASentinelCharacter* SentinelCharacter = Cast<ASentinelCharacter>(OtherActor);
	if (SentinelCharacter)
	{
		UBuffComponent* Buff = SentinelCharacter->GetBuff();
		if (Buff)
		{
			Buff->BuffJump(JumpZVelocityBuff, JumpBuffTime);
		}
	}
	Destroy();
}

