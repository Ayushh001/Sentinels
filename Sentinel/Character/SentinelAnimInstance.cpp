// Fill out your copyright notice in the Description page of Project Settings.


#include "SentinelAnimInstance.h"
#include "SentinelCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sentinel/Weapon/Weapon.h"
#include "Sentinel/SentinelTypes/CombatState.h"

void USentinelAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	SentinelCharacter = Cast<ASentinelCharacter>(TryGetPawnOwner());
}

void USentinelAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (SentinelCharacter == nullptr)
	{
		SentinelCharacter = Cast<ASentinelCharacter>(TryGetPawnOwner());
	}
	if (SentinelCharacter == nullptr) return;

	FVector Velocity = SentinelCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = SentinelCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = SentinelCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
	bWeaponEquipped = SentinelCharacter->IsWeaponEquipped();
	EquippedWeapon = SentinelCharacter->GetEquippedWeapon();
	bIsCrouched = SentinelCharacter->bIsCrouched;
	bAiming = SentinelCharacter->IsAiming();
	TurningInPlace = SentinelCharacter->GetTurningInPlace();
	bRotateRootBone = SentinelCharacter->ShouldRotateRootBone();
	bElimmed = SentinelCharacter->IsElimmed();

	//Offset Yaw for Strafing

	FRotator AimRotation = SentinelCharacter->GetBaseAimRotation();
	FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(SentinelCharacter->GetVelocity());
	FRotator DeltaRot = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation);
	DeltaRotation = FMath::RInterpTo(DeltaRotation, DeltaRot, DeltaTime, 10.f);
	YawOffset = DeltaRotation.Yaw;

	CharacterRotationLastFrame = CharacterRotation;
	CharacterRotation = SentinelCharacter->GetActorRotation();
	const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);
	const float Target = Delta.Yaw / DeltaTime;
	const float Interp = FMath::FInterpTo(Lean, Target, DeltaTime, 10.f);
	Lean = FMath::Clamp(Interp, -90.f, 90.f);

	AO_Yaw = SentinelCharacter->GetAO_Yaw();
	AO_Pitch = SentinelCharacter->GetAO_Pitch();

	if (bWeaponEquipped && EquippedWeapon && EquippedWeapon->GetWeaponMesh() && SentinelCharacter->GetMesh())
	{
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), ERelativeTransformSpace::RTS_World);
		FVector OutPosition;
		FRotator OutRotation;
		SentinelCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition);
		LeftHandTransform.SetRotation(FQuat(OutRotation));

		if (SentinelCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			FTransform RightHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("hand_r"), ERelativeTransformSpace::RTS_World);
			FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - SentinelCharacter->GetHitTarget()));
			RightHandRotation = FMath::RInterpTo(RightHandRotation, LookAtRotation, DeltaTime, 30.f);

		}
	}

	bUseFABRIK = SentinelCharacter->GetCombatState() == ECombatState::ECS_Unoccupied;
	bool bFARBRIKOverride = SentinelCharacter->IsLocallyControlled() &&
		SentinelCharacter->GetCombatState() != ECombatState::ECS_ThrowingGrenade && 
		SentinelCharacter->bFinishedSwapping;
	if (bFARBRIKOverride)
	{
		bUseFABRIK = !SentinelCharacter->bIsLocallyReloading();
	}
	bUseAimOffsets = SentinelCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !SentinelCharacter->GetDisableGameplay();
	bTransformRightHand = SentinelCharacter->GetCombatState() == ECombatState::ECS_Unoccupied && !SentinelCharacter->GetDisableGameplay();
}

