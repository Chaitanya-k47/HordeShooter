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


// Sets default values
AHordeShooterWeapon::AHordeShooterWeapon()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);

	Magazine = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Magazine"));
	Magazine->SetupAttachment(Mesh, FName("Magazine"));
	Magazine->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BarrelSmokeComp = CreateDefaultSubobject<UNiagaraComponent>(TEXT("BarrelSmokeComponent"));
	BarrelSmokeComp->SetupAttachment(Mesh, FName("SOC_MuzzleFlash"));
	BarrelSmokeComp->bAutoActivate = false; //keep it off by default
}


// Called when the game starts or when spawned
void AHordeShooterWeapon::BeginPlay()
{
	Super::BeginPlay();

	CurrentAmmo = MagSize;
	
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

}


void AHordeShooterWeapon::StartFire()
{
	if(!bIsEquipped || !bCanFire || bIsReloading) return;

	if(CurrentAmmo<=0)
	{
		Reload();
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
		Reload();
		return;
	}

	if(!bCanFire)
	{
		StopFire();
		return;
	}

	if(!CurrentOwner || !CurrentOwner->FirstPersonCamera) return;

	//consume ammo
	CurrentAmmo--;

	//broadcast
	OnAmmoChanged.Broadcast(CurrentAmmo, MagSize);

	//dynamic smoke heat tracking:
	BarrelSmokeComp->Deactivate();
	GetWorldTimerManager().ClearTimer(SmokeTimerHandle);
	
	BulletsFiredConsecutively++;
	
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
	if (MuzzleFlashSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAttached(
			MuzzleFlashSystem,
			Mesh, // The weapon mesh
			FName("SOC_MuzzleFlash"), // The exact name of your socket!
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true // Auto-destroy when the particle finishes playing
		);
	}

	//Casing Ejection:
	if(CasingClass)
	{
		FTransform SocketTransform = Mesh->GetSocketTransform(FName("SOC_CasingEjection"));

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = CurrentOwner;

		//spawn the casings(its beginplay() handles the physics impulse.)
		AHordeShooterCasing* SpawnedCasing = GetWorld()->SpawnActor<AHordeShooterCasing>(
			CasingClass,
			SocketTransform.GetLocation(),
			SocketTransform.GetRotation().Rotator(),
			SpawnParams
		);

		//Feed the player's ANTICIPATED velocity to the casing.
		if(SpawnedCasing && CurrentOwner)
		{
			// FVector InheritedVelocity = CurrentOwner->GetVelocity();
			// FVector InputDir = CurrentOwner->GetLastMovementInputVector();

			// if(CurrentOwner->GetCharacterMovement()->IsMovingOnGround() && !InputDir.IsNearlyZero())
			// {
			// 	float MaxSpeed = FMath::Max(CurrentOwner->GetCharacterMovement()->MaxWalkSpeed, InheritedVelocity.Size());
			// 	InheritedVelocity = FVector(InputDir.X * MaxSpeed, InputDir.Y * MaxSpeed, InheritedVelocity.Z);
			// }

			// SpawnedCasing->EjectCasing(InheritedVelocity);

			SpawnedCasing->EjectCasing(CurrentOwner->GetVelocity());
		}

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

		//impact fx:
		if(ImpactSystem)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				ImpactSystem,
				HitResult.ImpactPoint,
				HitRotation
			);
		}

		//bullet hole decal:
		if(BulletHoleDecal)
		{
			UDecalComponent* DecalComp = UGameplayStatics::SpawnDecalAtLocation(
				GetWorld(),
				BulletHoleDecal,
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
		if(ImpactSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				GetWorld(),
				ImpactSound,
				HitResult.ImpactPoint
			);
		}

		//apply damage:
		if(HitResult.GetActor())
		{
			//TODO: DamagableInterface Logic here.
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
	if(CurrentAmmo == MagSize || bIsReloading) return;
	
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

	bIsReloading = false;
	GetWorldTimerManager().ClearTimer(ReloadTimerHandle);
}



void AHordeShooterWeapon::FinishReload()
{
	CurrentAmmo = MagSize;
	bIsReloading = false;

	//broadcast
	OnAmmoChanged.Broadcast(CurrentAmmo, MagSize);
}


void AHordeShooterWeapon::StartAltFire()
{
	//base implementation empty, child classes can override this for alt fire
}


void AHordeShooterWeapon::StopAltFire()
{
	//base implementation empty, child classes can override this for alt fire
}






