#include "RodmasterEnemy.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
// Fill out your copyright notice in the Description page of Project Settings.


#include "RodmasterEnemy.h"

ARodmasterEnemy::ARodmasterEnemy()
{
    MaxHealth = 800.f;
    AttackRange = 4000.f;
    WalkSpeed = 250.f;
    SprintSpeed = 250.f;

    bAlwaysFacePlayer = true;
    bStopToAttack = true;
    bStrafeDuringCooldown = true;
    bChargeOnPlayerWhileAttacking = false;
    AttackCooldown = 3.f;

    GetCharacterMovement()->RotationRate = FRotator(0.0f, 200.0f, 0.0f);
}

void ARodmasterEnemy::BeginPlay()
{
    Super::BeginPlay();
    BaseWalkSpeed = WalkSpeed;
    BaseAttackDamage = AttackDamage;
    BaseCloseMeleeDamage = CloseMeleeDamage;
    BaseCloseSlamDamage = CloseSlamDamage;

    //register the custom attack montages with the AttackMontages array of base class
	//this ensures OnMontageEnded correctly resets bIsAttacking to false
	for(UAnimMontage* Montage : RangedPlasmaMontages)
	{
		if(Montage) AttackMontages.AddUnique(Montage);
	}
	
	for(UAnimMontage* Montage : CloseSlamMontages)
	{
		if(Montage) AttackMontages.AddUnique(Montage);
	}
}

void ARodmasterEnemy::ActivateEnemy(const FTransform& SpawnTransform, const TArray<float>& DifficultyMultipliers)
{
    Super::ActivateEnemy(SpawnTransform, DifficultyMultipliers);

    CurrentOvercharge = 0.0f;
	bIsExploding = false;
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	UpdateOverchargeVisuals(0.0f);//reset glow
}

bool ARodmasterEnemy::ReactToHit(float DamageAmount, const FVector &HitImpulse, FName HitBoneName, FName DamageSource)
{
    if(bIsDead || bIsExploding) return false;

    //if energy based, then overcharge
    if(DamageSource == FName("RayGun") || DamageSource == FName("RayGunAlt") || DamageSource == FName("Lightning"))
    {
        CurrentOvercharge += DamageAmount;

        float ChargeRatio = FMath::Clamp(CurrentOvercharge/MaxOvercharge, 0.f, 1.f);

        //scale speed:
        GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * FMath::Lerp(1.f, MaxSpeedMultiplier, ChargeRatio);

        //scale attack damage:
        float AttackMultiplier = FMath::Lerp(1.f, MaxDamageMultiplier, ChargeRatio);
        AttackDamage = BaseAttackDamage * AttackMultiplier;
        CloseMeleeDamage = BaseCloseMeleeDamage * AttackMultiplier;
        CloseSlamDamage = BaseCloseSlamDamage * AttackMultiplier;

        //visuals:
        UpdateOverchargeVisuals(ChargeRatio);

        if(CurrentOvercharge >= MaxOvercharge) TriggerOverchargeExplosion();

        //no health damage(physical damage hence return false)
        return false;
    }

    //if bullet hit or melee hit:
    float ResistedDamage = DamageAmount;
    if(DamageSource == FName("Slam") || DamageSource == FName("Melee") || DamageSource == NAME_None)
	{
		ResistedDamage *= BulletDamageMultiplier; // 75% reduction
	}

    return Super::ReactToHit(ResistedDamage, HitImpulse, HitBoneName, DamageSource);
}

void ARodmasterEnemy::PerformAttack()
{
    if(bIsAttacking || bIsStunned || bIsDead) return;

    APawn* PlayerTarget = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if(!PlayerTarget) return;

    bIsAttacking = true;

    float DistSq = FVector::DistSquared(GetActorLocation(), PlayerTarget->GetActorLocation());
    float CloseRangeSq = FMath::Square(CloseSlamRange);

    UAnimMontage* MontageToPlay = nullptr;

    //close range attacks
    if(DistSq <= CloseRangeSq && CloseSlamMontages.Num() > 0)
    {
        MontageToPlay = CloseSlamMontages[FMath::RandRange(0, CloseSlamMontages.Num() - 1)];
    } 

    //long range attacks
    else if(RangedPlasmaMontages.Num() > 0) 
    {
        MontageToPlay = RangedPlasmaMontages[FMath::RandRange(0, RangedPlasmaMontages.Num() - 1)];
    }

    //anim speed scaling
    if(MontageToPlay && GetMesh()->GetAnimInstance())
	{
		float AnimSpeed = 1.0f;
		float ChargeRatio = FMath::Clamp(CurrentOvercharge / MaxOvercharge, 0.0f, 1.0f);
		AnimSpeed = FMath::Lerp(1.0f, MaxDamageMultiplier, ChargeRatio);
		
		GetMesh()->GetAnimInstance()->Montage_Play(MontageToPlay, AnimSpeed);
	}
}

void ARodmasterEnemy::TriggerOverchargeExplosion()
{
    bIsExploding = true;
    FVector ExplodeLoc = GetActorLocation();

    //FX
    if(ExplosionVFX) UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionVFX, ExplodeLoc);
    if(ExplosionSFX) UGameplayStatics::PlaySoundAtLocation(GetWorld(), ExplosionSFX, ExplodeLoc);

    //aoe damage:
    TArray<FOverlapResult> OverlapResults;
    FCollisionShape SphereCol = FCollisionShape::MakeSphere(ExplosionRadius);
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    bool bHasOverlap = GetWorld()->OverlapMultiByObjectType(
        OverlapResults,
        ExplodeLoc,
        FQuat::Identity, 
        ObjectQueryParams,
        SphereCol,
        QueryParams
    );

    if(bHasOverlap)
    {
        TSet<AActor*> DamagedActors;

        for(const FOverlapResult& Overlap : OverlapResults)
        {
            AActor* HitActor = Overlap.GetActor();
            if(HitActor && !DamagedActors.Contains(HitActor) && HitActor->GetClass()->ImplementsInterface(UDamageableInterface::StaticClass()))
            {
                DamagedActors.Add(HitActor);
                IDamageableInterface* Damageable = Cast<IDamageableInterface>(HitActor);

                FVector PushDirection = (HitActor->GetActorLocation() - ExplodeLoc).GetSafeNormal();
				PushDirection.Z += 0.8f;
                PushDirection.Normalize();

                Damageable->ReactToHit(ExplosionDamage, PushDirection * ExplosionImpulse, NAME_None, FName("Overcharge"));
            }
        }
    }

    //instantly kill rodmaster
    ReactToHit(MaxHealth + 500, FVector::ZeroVector, NAME_None, FName("Overcharge"));
}

void ARodmasterEnemy::UpdateOverchargeVisuals(float OverchargeRatio)
{
    // C++ to Blueprint hook. You can use this to drive a Material Instance Dynamic (MID) 
	// to make his veins glow bright red/orange as he charges up.
	if (GetMesh())
	{
		// Example if you have a MID set up:
		// DynamicMat->SetScalarParameterValue(FName("OverchargeGlow"), OverchargeRatio * 50.0f);
	}
}
