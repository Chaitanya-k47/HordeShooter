// Fill out your copyright notice in the Description page of Project Settings.


#include "RayGun.h"

#include "HordeShooterCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/AudioComponent.h"
#include "Components/DecalComponent.h"
#include "DamageableInterface.h"
#include "ArenaManager.h"
#include "PlayerProgressionComponent.h"

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

    ChargeLoopAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("ChargeLoopAudioComp"));
	ChargeLoopAudioComp->SetupAttachment(Mesh, FName("SOC_MuzzleFlash"));
	ChargeLoopAudioComp->bAutoActivate = false;
}

void ARayGun::BeginPlay()
{
    Super::BeginPlay();
    
    if(Mesh)
    {
        DynamicWeaponMat = Mesh->CreateAndSetMaterialInstanceDynamic(0);
    }

    AActor* ArenaActor = UGameplayStatics::GetActorOfClass(GetWorld(), AArenaManager::StaticClass());
	if (ArenaActor) CachedArenaManager = Cast<AArenaManager>(ArenaActor);
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
            CurrentBeamHitBone = Hit.BoneName; //cache the bone that is being hit

            //Scorchmark pedometer:
            if(ScorchDecalMaterial)
            {
                float DistanceMoved = FVector::Dist(LastScorchLocation, Hit.Location);

				//if we moved far enough (or if it's the very first frame we hit the wall)
				if (DistanceMoved > ScorchSpawnDistance || LastScorchLocation.IsZero())
				{
                    // 2. RANDOMIZE ROTATION: Spin the decal like a roulette wheel
					FRotator DecalRotation = Hit.ImpactNormal.Rotation();
					DecalRotation.Roll = FMath::RandRange(0.0f, 360.0f); // Random spin!

					// 3. RANDOMIZE SIZE: Slightly vary the size so the trail isn't a perfect uniform width
					float RandomSize = FMath::RandRange(ScorchDecalSize * 0.7f, ScorchDecalSize * 1.3f);
					FVector DecalScale = FVector(RandomSize, RandomSize, RandomSize);

					UDecalComponent* DecalComp = UGameplayStatics::SpawnDecalAtLocation(
						GetWorld(),
						ScorchDecalMaterial,
						FVector(ScorchDecalSize, ScorchDecalSize, ScorchDecalSize),
						Hit.Location,
						Hit.ImpactNormal.Rotation(),
						ScorchLifespan
					);

                    if (DecalComp)
					{
						//parameters: StartDelay (0s), FadeDuration (ScorchLifespan), bDestroyOwnerAfterFade (false)
						//this makes Decal Lifetime opcity go from 1.0 to 0.0 over the total lifespan
						DecalComp->SetFadeOut(0.0f, ScorchLifespan, false);
					}

					//remember this spot for the next distance check
					LastScorchLocation = Hit.Location;
				}
            }
		}
		else
		{
			BeamImpactComponent->SetWorldLocation(FVector(0,0,-10000)); //hide in the floor
            BeamImpactAudioComp->SetVolumeMultiplier(0.0f); //mute
			CurrentBeamTarget = nullptr;

            LastScorchLocation = FVector::ZeroVector; //reset scorch pedometer when not hitting anything
		}
	}

    //ALT FIRE charge tracker:
    if(bIsCharging && bCanFire && !bIsLowered)
    {
        CurrentChargeTime += DeltaTime;

        float ChargeRatio = FMath::Clamp(CurrentChargeTime / ChargeTimeRequired, 0.0f, 1.0f);
        if(ChargeOrbComp) 
        {
            ChargeOrbComp->SetFloatParameter(FName("OrbSize"), ChargeRatio*15.0f);
            ChargeOrbComp->SetFloatParameter(FName("EmberSphereRadius"), ChargeRatio*10.5f); 
        }

        //OVERCHARGE PENALTY: If the player holds the charge too long, they have a chance to be struck by lightning
        if(CachedArenaManager->bIsCombatActive && CurrentChargeTime >= ChargeTimeRequired + MaxOverchargeTime && !bHasRolledOvercharge)
        {
            bHasRolledOvercharge = true; //only roll once per charge cycle

            if(FMath::FRand() <= OverchargeStrikeProbability)
            {
                if(CachedArenaManager && CurrentOwner)
				{
                    FVector PlayerLoc = CurrentOwner->GetActorLocation();
                    FVector CamForward = CurrentOwner->GetViewRotation().Vector().GetSafeNormal2D();
                    FVector CamRight = FVector::CrossProduct(FVector::UpVector, CamForward);

                    float ForwardDist = FMath::RandRange(200.0f, 400.0f);
                    float RightDist = FMath::RandRange(-150.0f, 150.0f);
                    FVector StrikeOffset = (CamForward*ForwardDist) + (CamRight*RightDist);
                    FVector TargetXY = PlayerLoc + StrikeOffset;

                    //do a line trace from above the player to the ground to find the floor Z.
                    FHitResult Hit;
                    FVector TraceStart = FVector(TargetXY.X, TargetXY.Y, PlayerLoc.Z + 2000.f);
                    FVector TraceEnd = FVector(TraceStart.X, TraceStart.Y, TraceStart.Z - 10000.f);
                    FCollisionQueryParams Params;
                    Params.AddIgnoredActor(CurrentOwner);

                    if(GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
                    {
                        CachedArenaManager->ExecuteLightningStrike(Hit.ImpactPoint);
                    }
                }
            }
        }
    }
    else if(bIsCharging && (!bCanFire || bIsLowered))
    {
        StopAltFire(); //stop charging if we lose the ability to fire
    }
    
    //dynamic weapon glow:
    // if(DynamicWeaponMat)
    // {
    //     float TargetGlow = 0.0f;

	// 	if (bIsCharging && bCanFire)
	// 	{
	// 		float ChargePercent = FMath::Clamp(CurrentChargeTime / ChargeTimeRequired, 0.0f, 1.0f);
	// 		TargetGlow = FMath::Lerp(0.0f, 50.0f, ChargePercent); // Target brightens as you charge
	// 	}

	// 	// Smoothly fade up when charging, and fade down to 0 when released!
	// 	CurrentGlow = FMath::FInterpTo(CurrentGlow, TargetGlow, DeltaTime, 10.0f); 
	// 	DynamicWeaponMat->SetScalarParameterValue(FName("GlowIntensity"), CurrentGlow);
    // }

}

void ARayGun::StartFire()
{
    if (bIsReloading || bIsFiringBeam || bIsCharging || !bCanFire || bIsLowered) return;

    if(CurrentAmmo < BeamAmmoCostPerBeamTick)
    {
        if(TotalAmmoReserve > 0) Reload();
        else
        {
            //play empty click sound
            if(ClipEmptySound)
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), ClipEmptySound, GetActorLocation());
			}
        }
        return;
    }
   
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

    bHasRolledOvercharge = false; //reset overcharge roll for next time
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
    if(bIsFiringBeam && (!bCanFire || bIsLowered))
    {
        StopFire();
        return;
    }

    if(CurrentAmmo < BeamAmmoCostPerBeamTick)
	{
		StopFire();

		if(TotalAmmoReserve > 0) Reload();
		return;
	}

    CurrentAmmo -= BeamAmmoCostPerBeamTick;
	OnAmmoChanged.Broadcast(CurrentAmmo, TotalAmmoReserve);

    if (CurrentBeamTarget && CurrentOwner && CurrentOwner->FirstPersonCamera)
	{
		if (CurrentBeamTarget->GetClass()->ImplementsInterface(UDamageableInterface::StaticClass()))
		{
			IDamageableInterface* DamageableActor = Cast<IDamageableInterface>(CurrentBeamTarget);
			if (DamageableActor)
			{
                float DamageMultiplier = 1.0f;
				if(CurrentOwner && CurrentOwner->ProgressionComponent)
				{
					DamageMultiplier = CurrentOwner->ProgressionComponent->GetDamageMultiplier();
				}

				FVector PushDirection = CurrentOwner->FirstPersonCamera->GetForwardVector();
                FVector FinalImpulse = PushDirection * (ShotImpulse * 0.2f);
				DamageableActor->ReactToHit(BeamDamagePerBeamTick * DamageMultiplier, FinalImpulse, CurrentBeamHitBone);
			}
		}
	}
}

void ARayGun::StartAltFire()
{
    if(bIsReloading || bIsCharging || bIsFiringBeam || !bCanFire || !bCanAltFire) return;

    if(CurrentAmmo < AltFireAmmoCost)
    {
        if(TotalAmmoReserve > 0) Reload();
		else
        { 
            if(ClipEmptySound)
			{
				UGameplayStatics::PlaySoundAtLocation(GetWorld(), ClipEmptySound, GetActorLocation());
			}
        }
		
        return;
    }

    bIsCharging = true;
    CurrentChargeTime = 0.0f;

    ChargeOrbComp->Activate(true);
    float PitchMultiplier = BaseChargeSoundDuration / ChargeTimeRequired;

	if(ChargeAudioComp->Sound)
    {
        ChargeAudioComp->SetFloatParameter(FName("ChargePitch"), PitchMultiplier);
        ChargeAudioComp->Play();
    }
    GetWorldTimerManager().SetTimer(ChargeLoopAudioTimerHandle, this, &ARayGun::StartChargeLoopAudio, ChargeTimeRequired, false);

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

void ARayGun::StartChargeLoopAudio()
{
    if(bIsCharging && ChargeLoopAudioComp->Sound)
	{
		ChargeLoopAudioComp->Play();
	}
}

void ARayGun::StopAltFire()
{
    if(!bIsCharging) return;

    bIsCharging = false;

    ChargeOrbComp->Deactivate();
	ChargeAudioComp->FadeOut(0.1f, 0.0f);
    ChargeLoopAudioComp->FadeOut(0.1f, 0.0f);

    //stop charge animation:
    if(CurrentOwner && CurrentOwner->CharacterArms && ArmsChargeLoopMontage)
    {
        UAnimInstance* ArmsAnimInstance = CurrentOwner->CharacterArms->GetAnimInstance();
        if(ArmsAnimInstance)
        {
            ArmsAnimInstance->Montage_Stop(0.1f, ArmsChargeLoopMontage);
        }   
    }

    if(CurrentChargeTime >= ChargeTimeRequired && (bCanFire && !bIsLowered))
    {
        PerformAltFire();
    }
    CurrentChargeTime = 0.0f; //reset charge time
}

void ARayGun::OnHolstered()
{
    Super::OnHolstered();

    bHasRolledOvercharge = false; //reset overcharge roll for next time
	bIsCharging = false;
	CurrentChargeTime = 0.0f; //reset to 0 so StopAltFire doesnt trigger the blast
	
	ChargeOrbComp->Deactivate();
	ChargeAudioComp->FadeOut(0.1f, 0.0f);
    ChargeLoopAudioComp->FadeOut(0.1f, 0.0f);

	//shut down the continuous beam if it was running
	bIsFiringBeam = false;
	CurrentBeamTarget = nullptr;
	BeamComponent->Deactivate();
	BeamImpactComponent->Deactivate();
	GetWorldTimerManager().ClearTimer(BeamDamageTimerHandle);
}

void ARayGun::Reload()
{
    if(CurrentAmmo == MagSize || bIsReloading) return;

    //if already charging, kill the charge then reload.
    if(bIsCharging)
	{
		bIsCharging = false;
		CurrentChargeTime = 0.0f;

		ChargeOrbComp->Deactivate();
		ChargeAudioComp->FadeOut(0.1f, 0.0f);

		if (CurrentOwner && CurrentOwner->CharacterArms && ArmsChargeLoopMontage)
		{
			UAnimInstance* ArmsAnimInstance = CurrentOwner->CharacterArms->GetAnimInstance();
			if (ArmsAnimInstance)
			{
				ArmsAnimInstance->Montage_Stop(0.1f, ArmsChargeLoopMontage);
			}
		}
	}

    //call the base class reload to handle the actual ammo refill and HUD update
	Super::Reload();
}


void ARayGun::PerformAltFire()
{
    //cunsume ammo
    CurrentAmmo -= AltFireAmmoCost;
    if(CurrentAmmo < 0) CurrentAmmo = 0;

    //broadcast
    OnAmmoChanged.Broadcast(CurrentAmmo, TotalAmmoReserve);

    //alt fire cooldown:
    float CooldownDuration = 0.0f;
    if(CurrentOwner && CurrentOwner->CharacterArms && ArmsDischargeMontage)
    {
        UAnimInstance* ArmsAnimInstance = CurrentOwner->CharacterArms->GetAnimInstance();
        if(ArmsAnimInstance)
        {
            ArmsAnimInstance->Montage_Play(ArmsDischargeMontage);
            CooldownDuration += ArmsDischargeMontage->GetPlayLength();
        }   
    }

    bCanAltFire = false;
    GetWorldTimerManager().SetTimer(AltFireCooldownTimerHandle, this, &ARayGun::ResetAltFireCooldown, CooldownDuration, false);

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

    FCollisionObjectQueryParams ObjectQueryParams;
    ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

    bool bHasOverlaps = GetWorld()->OverlapMultiByObjectType(
        OverlapResults,
        ImpactPoint,
        FQuat::Identity,
        ObjectQueryParams, //only look for enemies(pawns), ignore walls and other objects
        SphereCol,
        QueryParams
    );

    if(bHasOverlaps)
    {
        TSet<AActor*> DamagedActors;

        for(const FOverlapResult& Overlap : OverlapResults)
        {
            //damageableinterface implementation
            AActor* HitActor = Overlap.GetActor();

            if(HitActor && !DamagedActors.Contains(HitActor))
            {
                if(HitActor && HitActor->GetClass()->ImplementsInterface(UDamageableInterface::StaticClass()))
                {
                    IDamageableInterface* DamageableActor = Cast<IDamageableInterface>(HitActor);
                    if(DamageableActor)
                    {   
                        DamagedActors.Add(HitActor);

                        float DamageMultiplier = 1.0f;
                        if(CurrentOwner && CurrentOwner->ProgressionComponent)
                        {
                            DamageMultiplier = CurrentOwner->ProgressionComponent->GetDamageMultiplier();
                        }

                        //push the ememies out from the centre of the blast
                        FVector PushDirection = (HitActor->GetActorLocation() - ImpactPoint).GetSafeNormal();
                        FVector FinalImpulse = PushDirection * AltFireImpulse;
                        DamageableActor->ReactToHit(AltFireDamage * DamageMultiplier, FinalImpulse, NAME_None);
                    }
                }
            }
        }
    }

    if (AltFireMuzzleFlash)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			AltFireMuzzleFlash, Mesh, FName("SOC_MuzzleFlash"), 
			FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::SnapToTarget, true
		);
	}

    if(AltFireDischargeSound)
    {
        UGameplayStatics::PlaySoundAtLocation(GetWorld(), AltFireDischargeSound, Start);
    }

    if(AoEBlastEffect)
    {
        float ScaleFactor = (AltFireRadius / 100.0f); //assuming the Niagara system is designed with a base radius of 100 units

        UNiagaraComponent* BlastComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            GetWorld(),
            AoEBlastEffect,
            ImpactPoint,
            FRotator::ZeroRotator
        );

        if(BlastComp)
        {
            BlastComp->SetFloatParameter(FName("BlastScale"), ScaleFactor);
        }
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

void ARayGun::ResetAltFireCooldown()
{
    bCanAltFire = true;
}
