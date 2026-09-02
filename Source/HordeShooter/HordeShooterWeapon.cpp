// Fill out your copyright notice in the Description page of Project Settings.


#include "HordeShooterWeapon.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "HordeShooterCharacter.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimMontage.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/DecalComponent.h"
#include "HordeShooterCasing.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DamageableInterface.h"
#include "Curves/CurveVector.h"


// Sets default values
AHordeShooterWeapon::AHordeShooterWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCastShadow(false);
	Mesh->SetRenderCustomDepth(true);

	Magazine = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Magazine"));
	Magazine->SetupAttachment(Mesh, FName("Magazine"));
	Magazine->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Magazine->SetCastShadow(false);
	Magazine->SetRenderCustomDepth(true);

	BarrelSmokeComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BarrelSmokeComponent"));
	BarrelSmokeComp->SetupAttachment(Mesh, FName("SOC_MuzzleFlash"));
	BarrelSmokeComp->bAutoActivate = false; //keep it off by default
	
	MuzzleFlashComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("MuzzleFlashComponent"));
	MuzzleFlashComp->SetupAttachment(Mesh, FName("SOC_MuzzleFlash"));
	MuzzleFlashComp->bAutoActivate = false;
}


// Called when the game starts or when spawned
void AHordeShooterWeapon::BeginPlay()
{
	Super::BeginPlay();

	CurrentAmmo = MagSize;

	//Casings pool initialization:
	if(CasingClass)
	{
		CasingPoolSize = MagSize;

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;

		for(int32 i=0; i<CasingPoolSize; i++)
		{
			AHordeShooterCasing* NewCasing = GetWorld()->SpawnActor<AHordeShooterCasing>(CasingClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
			if(NewCasing)
			{
				CasingPool.Add(NewCasing);
			}
		}
	}
	
	if(!CurrentOwner && !bIsEquipped)
	{
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
	}
}


// Called every frame
void AHordeShooterWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if(CurrentOwner && (!CurrentRecoilOffset.Equals(TargetRecoilOffset, 0.001f) || bIsRecoveringRecoil))
	{
		//if recovering, pull back the target to centre
		if(bIsRecoveringRecoil) TargetRecoilOffset = FVector2D::ZeroVector;
		
		//slightly fast for recoil, slow for recovery
		float InterpSpeed = bIsRecoveringRecoil ? RecoilRecoverySpeed : RecoilKickSpeed;
		
		FVector2D NextRecoil = FMath::Vector2DInterpTo(CurrentRecoilOffset, TargetRecoilOffset, DeltaTime, InterpSpeed);
		FVector2D DeltaRecoil = NextRecoil - CurrentRecoilOffset;

		//apply recoil:
		CurrentOwner->AddControllerPitchInput(DeltaRecoil.X);
		CurrentOwner->AddControllerYawInput(DeltaRecoil.Y);

		//save the state
		CurrentRecoilOffset = NextRecoil;

		//if recoil fully recovered then stop recovery:
		if(bIsRecoveringRecoil && CurrentRecoilOffset.IsNearlyZero(0.01f))
		{
			CurrentRecoilOffset = FVector2D::ZeroVector;
			bIsRecoveringRecoil = false;
		}
	}

}


void AHordeShooterWeapon::StartFire()
{
	if(!bIsEquipped || !bCanFire || bIsReloading || bIsLowered) return;

	if(CurrentAmmo<=0)
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

	BarrelSmokeComp->Deactivate();
	GetWorldTimerManager().ClearTimer(SmokeTimerHandle);

	//shoot the first bullet immediately
	PerformFire();

	if(bIsAutomatic)
		GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AHordeShooterWeapon::PerformFire, FireRate, true);
	
}


void AHordeShooterWeapon::StopFire()
{
	GetWorldTimerManager().ClearTimer(FireTimerHandle);

	//start recoil recovery delay timer, once it finishes the rcoil recovers
	if(!TargetRecoilOffset.IsNearlyZero())
	{
		GetWorldTimerManager().SetTimer(RecoilRecoveryTimerHandle, this, &AHordeShooterWeapon::StartRecoilRecovery, RecoilRecoveryDelay, false);
	}
}


void AHordeShooterWeapon::StartRecoilRecovery()
{
	bIsRecoveringRecoil = true;
}


void AHordeShooterWeapon::StopBarrelSmoke()
{
	BarrelSmokeComp->Deactivate();
}

void AHordeShooterWeapon::EvaluateAndPlaySmoke()
{
	float SmokeDuration = BulletsFiredConsecutively * SmokeMultiplier;

	if (SmokeDuration >= 2.0f)
	{
		//maximum Cap Check (FMath::Clamp restricts the value between 0 and 6)
		SmokeDuration = FMath::Clamp(SmokeDuration, 0.0f, 6.0f);
		BarrelSmokeComp->Activate(true);
		GetWorldTimerManager().SetTimer(SmokeTimerHandle, this, &AHordeShooterWeapon::StopBarrelSmoke, SmokeDuration, false);
	}

	BulletsFiredConsecutively = 0; //reset the counter
}

void AHordeShooterWeapon::PerformFire()
{
	if(CurrentAmmo <= 0)
	{
		StopFire();

		if(TotalAmmoReserve > 0) Reload();
		return;
	}

	if(!bCanFire || bIsReloading || bIsLowered)
	{
		StopFire();
		return;
	}

	if(!CurrentOwner || !CurrentOwner->FirstPersonCamera) return;

	//consume ammo
	CurrentAmmo--;

	//broadcast
	OnAmmoChanged.Broadcast(CurrentAmmo, TotalAmmoReserve);

	//dynamic smoke heat tracking:
	BarrelSmokeComp->Deactivate();
	GetWorldTimerManager().ClearTimer(SmokeTimerHandle);
	
	BulletsFiredConsecutively++;

	//apply recoil:
	if(RecoilCurve && CurrentOwner && CurrentOwner->Controller)
	{
		//cancel recovery if shot again
		bIsRecoveringRecoil = false;
		GetWorldTimerManager().ClearTimer(RecoilRecoveryTimerHandle); 

		//retrieve recoil vector for specific bullet no. in consecutive fire.
		FVector RecoilData = RecoilCurve->GetVectorValue(static_cast<float>(BulletsFiredConsecutively));

		float JitterX = FMath::RandRange(-RecoilJitter.X, RecoilJitter.X);
		float JitterY = FMath::RandRange(-RecoilJitter.Y, RecoilJitter.Y);

		float PitchKick = -(RecoilData.X + JitterX) * RecoilMultiplier; //negative pitch pushes camera up.
		float YawKick = (RecoilData.Y + JitterY) * RecoilMultiplier;

		TargetRecoilOffset.X += PitchKick;
		TargetRecoilOffset.Y += YawKick;
	}	

	//restart the 0.5s countdown. If we don't shoot again in 0.5s, it triggers EvaluateAndPlaySmoke().
	GetWorldTimerManager().SetTimer(SmokeEvalTimerHandle, this, &AHordeShooterWeapon::EvaluateAndPlaySmoke, SmokeEvalWindow, false);

	if (CurrentOwner && CurrentOwner->CharacterArms)
	{
		if (ArmsFireMontage)
		{
			UAnimInstance* ArmsAnimInstance = CurrentOwner->CharacterArms->GetAnimInstance();
			if (ArmsAnimInstance)
			{
				UAnimMontage* MotageToPlay = CurrentOwner->bIsAiming ? ArmsFireMontage_Aimed : ArmsFireMontage;
				ArmsAnimInstance->Montage_Play(MotageToPlay);
			}
		}

		if (WeaponFireMontage && Mesh->GetAnimInstance())
		{
			Mesh->GetAnimInstance()->Montage_Play(WeaponFireMontage);
		}
	}

	//Muzzle Flash:
	if (MuzzleFlashComp->GetAsset())
	{
		MuzzleFlashComp->Activate(true);
	}

	//Casing Ejection:
	if(CasingPool.Num() > 0)
	{
		FTransform SocketTransform = Mesh->GetSocketTransform(FName("SOC_CasingEjection"));

		//get current casing from the pool.
		AHordeShooterCasing* PooledCasing = CasingPool[CurrentCasingIndex];

		//activate and eject the casing:
		if(PooledCasing && CurrentOwner)
		{
			PooledCasing->ActivateAndEject(SocketTransform, CurrentOwner->GetVelocity());
		}

		//increment the casing index, and loop back to 0 if we reach the end of the pool(roundrobin)
		CurrentCasingIndex = (CurrentCasingIndex + 1) % CasingPoolSize;
	}

	//Shoot Sound
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShootSound, GetActorLocation());
	}

	//linetrace logic:
	FVector Start = CurrentOwner->FirstPersonCamera->GetComponentLocation();
	FVector ForwardVector = CurrentOwner->FirstPersonCamera->GetForwardVector();
	FVector End = Start + (ForwardVector * WeaponRange);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(CurrentOwner);
	QueryParams.bReturnPhysicalMaterial = true;

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		QueryParams
	);

	//Bullet Tracer:
	FVector TracerEndPoint = bHit ? HitResult.Location : End;
	if (TracerSystem)
	{
		// 1. Get the exact location of the gun barrel right now
		FTransform MuzzleTransform = Mesh->GetSocketTransform(FName("SOC_MuzzleFlash"));
		//FVector MuzzleLocation = Mesh->GetSocketLocation(FName("SOC_MuzzleFlash"));

		// 2. Spawn the beam starting at the muzzle
		UNiagaraComponent* TracerComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			TracerSystem,
			MuzzleTransform.GetLocation(), //+ FVector(10000.f, 0.f, 0.f),
			MuzzleTransform.Rotator(),
			FVector(1.f),
			true, // Auto destroy
			true, // Auto activate
			ENCPoolMethod::None,
			true // Pre-cull
		);

		// 3. Tell the Niagara System where the wall is!
		if (TracerComp)
		{
			TracerComp->SetVectorParameter(FName("TraceStart"), MuzzleTransform.GetLocation());
			TracerComp->SetVectorParameter(FName("TraceEnd"), TracerEndPoint);

			//CS2 style tracer:
			// TracerComp->SetVectorParameter(FName("TraceEnd"), TracerEndPoint);

			// FVector TracerDirection = (TracerEndPoint - MuzzleTransform.GetLocation()).GetSafeNormal();
			// FVector TracerVelocity = TracerDirection * 20000.f;
			// TracerComp->SetVectorParameter(FName("TracerVelocity"), TracerVelocity);
		}

	}

	//Impact:
	if(bHit)
	{
		FRotator HitRotation = HitResult.ImpactNormal.Rotation();

		// retrieve and store the surface type that was hit
		EPhysicalSurface SurfaceType = SurfaceType_Default;
		if(HitResult.PhysMaterial.IsValid())
		{
			SurfaceType = HitResult.PhysMaterial->SurfaceType;
		}

		//look up the effects bundle from dict or use default.
		FImpactEffects* EffectsToPlay = &DefaultImpact;
		if(SurfaceImpactEffects.Contains(SurfaceType))
		{
			EffectsToPlay = &SurfaceImpactEffects[SurfaceType];
		}

		//impact fx:
		if(EffectsToPlay->ImpactVFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				EffectsToPlay->ImpactVFX,
				HitResult.ImpactPoint,
				HitRotation
			);
		}

		//bullet hole decal:
		if(EffectsToPlay->ImpactDecal)
		{
			UDecalComponent* DecalComp = UGameplayStatics::SpawnDecalAtLocation(
				GetWorld(),
				EffectsToPlay->ImpactDecal,
				FVector(5.f, 5.f, 5.f), // decal size
				HitResult.ImpactPoint,
				HitRotation,
				10.f // decal lifespan
			);

			if(DecalComp)
			{
				UMaterialInstanceDynamic* DynamicDecalMat = DecalComp->CreateDynamicMaterialInstance();

				if(DynamicDecalMat)
				{
					float UniqueIndex = static_cast<float>(GetUniqueDecalIndex());
					DynamicDecalMat->SetScalarParameterValue(FName("AtlasIndex"), UniqueIndex);
				}
			}
		}

		//impact sound:
		if(EffectsToPlay->ImpactSFX)
		{
			UGameplayStatics::PlaySoundAtLocation(
				GetWorld(),
				EffectsToPlay->ImpactSFX,
				HitResult.ImpactPoint
			);
		}

		//apply damage:
		if(HitResult.GetActor())
		{
			//DamagableInterface logic:
			if(HitResult.GetActor()->GetClass()->ImplementsInterface(UDamageableInterface::StaticClass()))
			{
				IDamageableInterface* DamageableActor = Cast<IDamageableInterface>(HitResult.GetActor());
				if(DamageableActor)
				{
					FVector FinalImpulse = ForwardVector * ShotImpulse;
					bool bIsHeadshot = DamageableActor->ReactToHit(BaseDamage, FinalImpulse, HitResult.BoneName);

					if(bIsHeadshot && EffectsToPlay->HeadshotSound)
					{
						UGameplayStatics::PlaySoundAtLocation(GetWorld(), EffectsToPlay->HeadshotSound, HitResult.ImpactPoint);
					}
				}
			}
		}

	}


	if(!bIsAutomatic)
	{
		//if not automatic start cooldown immediately after shot
		bCanFire = false;
		GetWorldTimerManager().SetTimer(FireTimerHandle, this, &AHordeShooterWeapon::ResetFireCooldown, FireRate, false);
	}
}

int32 AHordeShooterWeapon::GetUniqueDecalIndex()
{
	if(AvailableDecalIndices.Num() == 0)
	{
		for(int32 i=0; i<TotalDecalVariations; ++i)
		{
			AvailableDecalIndices.Add(i);
		}
	}

	int32 RandomArrayPosition = FMath::RandRange(0, AvailableDecalIndices.Num() - 1);
	int32 ChosenAtlasIndex = AvailableDecalIndices[RandomArrayPosition];
	AvailableDecalIndices.RemoveAtSwap(RandomArrayPosition);
		
	return ChosenAtlasIndex;
}


void AHordeShooterWeapon::ResetFireCooldown()
{
	bCanFire = true;
}


void AHordeShooterWeapon::Reload()
{
	if(TotalAmmoReserve <= 0 || CurrentAmmo == MagSize || bIsReloading) return;
	
	bIsReloading = true;
	StopFire(); //cannot fire while reloading

	//reload time logic:
	float DynamicReloadTime = ReloadTime;
	if(CurrentOwner && CurrentOwner->CharacterArms)
	{
		if(ArmsReloadMontage)
		{
			UAnimInstance* ArmsAnimInstance = CurrentOwner->CharacterArms->GetAnimInstance();
			if(ArmsAnimInstance)
			{
				ArmsAnimInstance->Montage_Play(ArmsReloadMontage);
				
				//overwrite the timer duration with the exact length of the animation
				DynamicReloadTime = ArmsReloadMontage->GetPlayLength();
			}
		}

		if(WeaponReloadMontage && Mesh->GetAnimInstance())
		{
			Mesh->GetAnimInstance()->Montage_Play(WeaponReloadMontage);
		}
	}


	GetWorldTimerManager().SetTimer(ReloadTimerHandle, this, &AHordeShooterWeapon::FinishReload, DynamicReloadTime, false);
}

void AHordeShooterWeapon::OnHolstered()
{
	StopFire();

	bIsRecoveringRecoil = false;
	GetWorldTimerManager().ClearTimer(RecoilRecoveryTimerHandle);
	TargetRecoilOffset = FVector2D::ZeroVector;
	CurrentRecoilOffset = FVector2D::ZeroVector;

	bIsReloading = false;
	GetWorldTimerManager().ClearTimer(ReloadTimerHandle);

	if (Mesh && Mesh->GetAnimInstance())
	{
		//Montage_Stop with a 0.1s blend out. 
		//passing nullptr tells it to stop whatever montage is currently playing (Reload, Fire, etc.)
		Mesh->GetAnimInstance()->Montage_Stop(0.1f, nullptr);
	}
}

void AHordeShooterWeapon::RefillMagazine()
{
	//called by anim notif the exact moment when mag is inserted
	int32 AmmoNeeded = MagSize - CurrentAmmo; //how mush needed to fill the mag
	int32 AmmoToReload = FMath::Min(AmmoNeeded, TotalAmmoReserve); //how much we can actually reload based on reserve

	CurrentAmmo += AmmoToReload;
	TotalAmmoReserve -= AmmoToReload;
	
	OnAmmoChanged.Broadcast(CurrentAmmo, TotalAmmoReserve);
}

void AHordeShooterWeapon::FinishReload()
{
	bIsReloading = false;
	if(CurrentOwner) CurrentOwner->FinishReloading();
}


void AHordeShooterWeapon::StartAltFire()
{
	//base implementation empty, child classes can override this for alt fire
}


void AHordeShooterWeapon::StopAltFire()
{
	//base implementation empty, child classes can override this for alt fire
}






