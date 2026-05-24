// Fill out your copyright notice in the Description page of Project Settings.


#include "RayGun.h"

#include "HordeShooterCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/AudioComponent.h"

/*
    Decoupled logic for RayGun's primary fire(Beam):
    Tick() function handles the continuous beam visuals and charge-up glow effect.
    PerformBeamTick() function handles the actual damage application and ammo consumption, 
    called at intervals defined by BeamDamageRate while the beam is active.
*/


ARayGun::ARayGun()
{
    PrimaryActorTick.bCanEverTick = true;
    bCanAim = false;

    BeamMuzzleGlowComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BeamMuzzleGlowComp"));
	BeamMuzzleGlowComp->SetupAttachment(Mesh, FName("SOC_MuzzleFlash"));
	BeamMuzzleGlowComp->bAutoActivate = false;

    BeamAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("BeamAudioComp"));
	BeamAudioComp->SetupAttachment(Mesh, FName("SOC_MuzzleFlash"));
	BeamAudioComp->bAutoActivate = false;

    BeamImpactAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("BeamImpactAudioComp"));
	BeamImpactAudioComp->SetupAttachment(RootComponent);
	BeamImpactAudioComp->bAutoActivate = false;

    BeamComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BeamComponent"));
	BeamComponent->SetupAttachment(Mesh, FName("SOC_MuzzleFlash"));
	BeamComponent->bAutoActivate = false;

	BeamImpactComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BeamImpactComponent"));
	BeamImpactComponent->SetupAttachment(RootComponent);
	BeamImpactComponent->bAutoActivate = false;

    ChargeOrbComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ChargeOrbComp"));
	ChargeOrbComp->SetupAttachment(Mesh, FName("SOC_MuzzleFlash"));
	ChargeOrbComp->bAutoActivate = false;

	ChargeAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("ChargeAudioComp"));
	ChargeAudioComp->SetupAttachment(Mesh, FName("SOC_MuzzleFlash"));
	ChargeAudioComp->bAutoActivate = false;
}

void ARayGun::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    //PRIMARY FIRE, Beam visuals:
    if (bIsFiringBeam && CurrentOwner && CurrentOwner->FirstPersonCamera)
	{
		FVector Start = CurrentOwner->FirstPersonCamera->GetComponentLocation();
		FVector Forward = CurrentOwner->FirstPersonCamera->GetForwardVector();
		FVector End = Start + (Forward * WeaponRange);

		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		Params.AddIgnoredActor(CurrentOwner);

		bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);

		//update the Niagara Beam end point
		BeamComponent->SetVectorParameter(FName("TraceEnd"), bHit ? Hit.Location : End);

		//move the impact sparks to the wall
		if (bHit)
		{
			BeamImpactComponent->SetWorldLocation(Hit.Location);
			BeamImpactComponent->SetWorldRotation(Hit.ImpactNormal.Rotation());

            BeamImpactAudioComp->SetVolumeMultiplier(1.0f); //play

			CurrentBeamTarget = Hit.GetActor(); //cache the target for the damage timer
		}
		else
		{
			BeamImpactComponent->SetWorldLocation(FVector(0,0,-10000)); //hide in the floor

            BeamImpactAudioComp->SetVolumeMultiplier(0.0f); //mute

			CurrentBeamTarget = nullptr;
		}
	}

    //ALT FIRE charge tracker:
    if(bIsCharging)
    {
        CurrentChargeTime += DeltaTime;

        float ChargePercent = FMath::Clamp(CurrentChargeTime / ChargeTimeRequired, 0.0f, 1.0f); 

        ChargeOrbComp->SetFloatParameter(FName("ChargeProgress"), ChargePercent);

        if(UMaterialInstanceDynamic* DynamicMat = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0)))
        {
			float GlowValue = FMath::Lerp(1.0f, 50.0f, ChargePercent); 
			DynamicMat->SetScalarParameterValue(FName("GlowIntensity"), GlowValue);
        }
    }


}

void ARayGun::StartFire()
{
    if (CurrentAmmo < BeamAmmoCostPerBeamTick || bIsReloading || bIsFiringBeam || bIsCharging) return;
   
    bIsFiringBeam = true;

    if(BeamComponent && BeamImpactComponent)
    {
        BeamComponent->Activate(true);
        BeamImpactComponent->Activate(true);
    }
    
    BeamMuzzleGlowComp->Activate(true);
    if(BeamAudioComp->Sound) BeamAudioComp->Play();
    if(BeamImpactAudioComp->Sound) BeamImpactAudioComp->Play();

    if(CurrentOwner && CurrentOwner->CharacterArms && ArmsBeamLoopMontage)
	{
		UAnimInstance* ArmsAnimInstance = CurrentOwner->CharacterArms->GetAnimInstance();
		if (ArmsAnimInstance) ArmsAnimInstance->Montage_Play(ArmsBeamLoopMontage);
	}

    PerformBeamTick();
	GetWorldTimerManager().SetTimer(BeamDamageTimerHandle, this, &ARayGun::PerformBeamTick, BeamDamageRate, true);
    //beam tick happens every BeamDamageRate seconds, applying damage and consuming ammo as long as the beam is active. 

}

void ARayGun::StopFire()
{
    if (!bIsFiringBeam) return;

    bIsFiringBeam = false;

    BeamComponent->Deactivate();
	BeamImpactComponent->Deactivate();
    BeamMuzzleGlowComp->Deactivate();
	BeamAudioComp->FadeOut(0.1f, 0.0f);
    BeamImpactAudioComp->FadeOut(0.1f, 0.0f);

    GetWorldTimerManager().ClearTimer(BeamDamageTimerHandle);


    if (CurrentOwner && CurrentOwner->CharacterArms && ArmsBeamLoopMontage)
	{
		UAnimInstance* ArmsAnimInstance = CurrentOwner->CharacterArms->GetAnimInstance();
		if (ArmsAnimInstance) ArmsAnimInstance->Montage_Stop(0.1f, ArmsBeamLoopMontage);
	}
}

void ARayGun::PerformBeamTick()
{
    if (CurrentAmmo < BeamAmmoCostPerBeamTick)
	{
		StopFire();
		Reload();
		return;
	}

    CurrentAmmo -= BeamAmmoCostPerBeamTick;
	OnAmmoChanged.Broadcast(CurrentAmmo, MagSize);

    if(CurrentBeamTarget)
    {
        //TODO: apply damage to CurrentBeamTarget using BeamDamagePerBeamTick and the damage interface.
    }
}

void ARayGun::StartAltFire()
{
    if(CurrentAmmo < AltFireAmmoCost || bIsReloading || bIsCharging || bIsFiringBeam) return;

    bIsCharging = true;
    CurrentChargeTime = 0.0f;

    ChargeOrbComp->Activate(true);
	if (ChargeAudioComp->Sound) ChargeAudioComp->Play();

    //start charge animation:
    if(CurrentOwner && CurrentOwner->CharacterArms && ArmsChargeLoopMontage)
    {
        UAnimInstance* ArmsAnimInstance = CurrentOwner->CharacterArms->GetAnimInstance();
        if(ArmsAnimInstance)
        {
            ArmsAnimInstance->Montage_Play(ArmsChargeLoopMontage);
        }   
    }
}

void ARayGun::StopAltFire()
{
    if(!bIsCharging) return;

    bIsCharging = false;

    ChargeOrbComp->Deactivate();
	ChargeAudioComp->FadeOut(0.1f, 0.0f);

    //stop charge animation:
    if(CurrentOwner && CurrentOwner->CharacterArms && ArmsChargeLoopMontage)
    {
        UAnimInstance* ArmsAnimInstance = CurrentOwner->CharacterArms->GetAnimInstance();
        if(ArmsAnimInstance)
        {
            ArmsAnimInstance->Montage_Stop(0.1f, ArmsChargeLoopMontage);
        }   
    }

    //Reset gun glow immediately
	if (UMaterialInstanceDynamic* DynamicMat = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0)))
	{
		DynamicMat->SetScalarParameterValue(FName("GlowIntensity"), 1.0f);
	}

    if(CurrentChargeTime >= ChargeTimeRequired)
    {
        PerformAltFire();
    }
    CurrentChargeTime = 0.0f; //reset charge time
}


void ARayGun::PerformAltFire()
{
    //cunsume ammo
    CurrentAmmo -= AltFireAmmoCost;
    if(CurrentAmmo < 0) CurrentAmmo = 0;

    //broadcast
    OnAmmoChanged.Broadcast(CurrentAmmo, MagSize);

    if(CurrentOwner && CurrentOwner->CharacterArms && ArmsDischargeMontage)
    {
        UAnimInstance* ArmsAnimInstance = CurrentOwner->CharacterArms->GetAnimInstance();
        if(ArmsAnimInstance)
        {
            ArmsAnimInstance->Montage_Play(ArmsDischargeMontage);
        }   
    }

    FVector Start = CurrentOwner->FirstPersonCamera->GetComponentLocation();
    FVector ForwardVector = CurrentOwner->FirstPersonCamera->GetForwardVector();
    FVector End = Start + (ForwardVector * WeaponRange);

    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    QueryParams.AddIgnoredActor(CurrentOwner);

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        Start,
        End,
        ECC_Visibility,
        QueryParams
    );

    FVector ImpactPoint = bHit ? HitResult.Location : End;

    //AOE Damage logic:
    TArray<FOverlapResult> OverlapResults;
    FCollisionShape SphereCol = FCollisionShape::MakeSphere(AltFireRadius);

    bool bHasOverlaps = GetWorld()->OverlapMultiByChannel(
        OverlapResults,
        ImpactPoint,
        FQuat::Identity,
        ECC_Pawn, //only look for enemies(pawns), ignore walls and other objects
        SphereCol,
        QueryParams
    );

    if(bHasOverlaps)
    {
        for(const FOverlapResult& Overlap : OverlapResults)
        {
            AActor* HitActor = Overlap.GetActor();
            if(HitActor)
            {
                //TODO: call Damage interface
            }
        }
    }

    if(AoEBlastEffect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            AoEBlastEffect,
            ImpactPoint,
            FRotator::ZeroRotator
        );
    }

    if (AltFireMuzzleFlash)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			AltFireMuzzleFlash, Mesh, FName("SOC_MuzzleFlash"), 
			FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true
		);
	}

    if (AltFireBeamSystem)
	{
		FVector MuzzleLoc = Mesh->GetSocketLocation(FName("SOC_MuzzleFlash"));
		UNiagaraComponent* BlastBeam = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), AltFireBeamSystem, MuzzleLoc, FRotator::ZeroRotator
		);
		
		if (BlastBeam)
		{
			// Tell the blast beam where the wall is
			BlastBeam->SetVectorParameter(FName("TraceEnd"), ImpactPoint);
		}
	}

	if (AltFireBlastSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), AltFireBlastSound, Start);
	}
}
