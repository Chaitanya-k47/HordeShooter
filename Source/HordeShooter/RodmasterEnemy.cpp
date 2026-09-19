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
}

void ARodmasterEnemy::BeginPlay()
{
    Super::BeginPlay();
    BaseWalkSpeed = WalkSpeed;
    BaseAttackDamage = AttackDamage;
    BaseCloseMeleeDamage = CloseMeleeDamage;
    BaseCloseSlamDamage = CloseSlamDamage;
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
    
}

void ARodmasterEnemy::TriggerOverchargeExplosion()
{

}

void ARodmasterEnemy::UpdateOverchargeVisuals(float OverchargeRatio)
{
    
}
