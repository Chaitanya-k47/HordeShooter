#include "RodmasterEnemy.h"
#include "RodmasterEnemy.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "EnemyAIController.h"
#include "Components/CapsuleComponent.h"
// Fill out your copyright notice in the Description page of Project Settings.


#include "RodmasterEnemy.h"

ARodmasterEnemy::ARodmasterEnemy()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.SetTickFunctionEnable(false);

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
    BaseCloseSlamDamage = CloseSlamDamage;

    OriginalMeshZ = GetMesh()->GetRelativeLocation().Z;

    //register the custom attack montages with the AttackMontages array of base class
	//this ensures OnMontageEnded correctly resets bIsAttacking to false
	for(UAnimMontage* Montage : RangedPlasmaMontages)
	{
		if(Montage) AttackMontages.AddUnique(Montage);
	}
	
	if(CloseSlamMontage) AttackMontages.AddUnique(CloseSlamMontage);
	
    if(GetMesh())
	{
		DynamicGlowMat = GetMesh()->CreateAndSetMaterialInstanceDynamic(0);
	}
}


void ARodmasterEnemy::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    FVector CurrentLoc = GetMesh()->GetRelativeLocation();
    float NewZ = FMath::FInterpTo(CurrentLoc.Z, TargetMeshZ, DeltaTime, 20.f);
    GetMesh()->SetRelativeLocation(FVector(CurrentLoc.X, CurrentLoc.Y, NewZ));

    if(bIsLandingRecovery && FMath::IsNearlyEqual(NewZ, OriginalMeshZ, 0.5f))
    {
        bIsLandingRecovery = false; //turn off recovery flag
        SetActorTickEnabled(false); //turn off tick
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
        CloseSlamDamage = BaseCloseSlamDamage * AttackMultiplier;

        //visuals:
        UpdateOverchargeVisuals(ChargeRatio);

        if(CurrentOvercharge >= MaxOvercharge) StartOverchargeSequence();

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
    if(bIsAttacking || bIsStunned || bIsDead || bIsExploding) return;

    APawn* PlayerTarget = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if(!PlayerTarget) return;

    bIsAttacking = true;

    float DistSq = FVector::DistSquared(GetActorLocation(), PlayerTarget->GetActorLocation());
    float CloseRangeSq = FMath::Square(CloseSlamRange);

    UAnimMontage* MontageToPlay = nullptr;

    //close range attacks
    if(DistSq <= CloseRangeSq && CloseSlamMontage)
    {
        MontageToPlay = CloseSlamMontage;
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

void ARodmasterEnemy::TriggerExplosion(EExplosionType ExplosionType)
{
    FVector ExplodeLoc = GetActorLocation();
    TArray<FOverlapResult> OverlapResults;
    FCollisionShape ColShape;
	FCollisionObjectQueryParams ObjectQueryParams;
    FCollisionQueryParams QueryParams;

    float ExplosionDamage = 0.f;
    float ExplosionImpulse = 0.f;
    FName InDamageSource = NAME_None;
    bool KillRodmaster = false;

    switch(ExplosionType)
    {
    case EExplosionType::Slam:
        ColShape = FCollisionShape::MakeSphere(CloseSlamRange);
        QueryParams.AddIgnoredActor(this);
        ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
        ExplosionDamage = CloseSlamDamage;
        ExplosionImpulse = CloseSlamImpulse;
        InDamageSource = FName("RodSlam");
        KillRodmaster = false;

        if(CloseSlamSFX) UGameplayStatics::PlaySoundAtLocation(GetWorld(), CloseSlamSFX, ExplodeLoc);
        if(CloseSlamVFX)
		{
			float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
			FVector FloorLocation = GetActorLocation() - FVector(0.0f, 0.0f, HalfHeight-5);

			UNiagaraComponent* Blast = UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), CloseSlamVFX, FloorLocation, FRotator::ZeroRotator);
			if(Blast)
			{
				//send the dynamically calculated radius to niagara material
				Blast->SetFloatParameter(FName("BlastScale"), CloseSlamRange*2); 
			}
        }
        if(SlamCameraShake)
        {
            UGameplayStatics::PlayWorldCameraShake(
                GetWorld(),
                SlamCameraShake,
                ExplodeLoc,
                SlamShakeInnerRadius,
                SlamShakeOuterRadius,
                1.f,
                false  
            );
        }       

        break;
    
    case EExplosionType::Overcharge:
        bIsExploding = true;
        ColShape = FCollisionShape::MakeSphere(OverchargeExplosionRadius);
        QueryParams.AddIgnoredActor(this);
        ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
        ExplosionDamage = OverchargeExplosionDamage;
        ExplosionImpulse = OverchargeExplosionImpulse;
        InDamageSource = FName("Overcharge");
        KillRodmaster = true;

        if(OverchargeExplosionSFX) UGameplayStatics::PlaySoundAtLocation(GetWorld(), OverchargeExplosionSFX, ExplodeLoc);
        if(OverchargeExplosionVFX) UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), OverchargeExplosionVFX, ExplodeLoc);

        break;

    default:
        return;
    }

    //FX
    // if(ExplosionVFXToPlay) UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ExplosionVFXToPlay, ExplodeLoc);
    // if(ExplosionSFXToPlay) UGameplayStatics::PlaySoundAtLocation(GetWorld(), ExplosionSFXToPlay, ExplodeLoc);

    bool bHasOverlap = GetWorld()->OverlapMultiByObjectType(
        OverlapResults,
        ExplodeLoc,
        FQuat::Identity, 
        ObjectQueryParams,
        ColShape,
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

                Damageable->ReactToHit(ExplosionDamage, PushDirection * ExplosionImpulse, NAME_None, InDamageSource);
            }
        }
    }

    //instantly kill rodmaster
    if(KillRodmaster) Super::ReactToHit(MaxHealth + 500.f, FVector::ZeroVector, NAME_None, InDamageSource);
}

void ARodmasterEnemy::ExecuteSlam()
{
    if(bIsDead || bIsStunned || bIsExploding) return;
    TriggerExplosion(EExplosionType::Slam);
}

void ARodmasterEnemy::ExecuteSlamJump()
{
    if(bIsDead || bIsStunned || bIsExploding) return;

    FVector JumpDirection = GetActorForwardVector();

    APawn* PlayerTarget = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if(PlayerTarget)
    {
        FVector ToPlayer = PlayerTarget->GetActorLocation() - GetActorLocation();
        ToPlayer.Z = 0.0f;  //flatten the vector(if there's a difference in Z coordinates of rodmaster and player this fixes that)
        
        JumpDirection = ToPlayer.GetSafeNormal();
    }

    JumpDirection.Z += 1.4;
    JumpDirection.Normalize();
	FVector CalculatedVelocity = JumpDirection * LaunchSpeed;

    //Calculate how far down the mesh needs to slide.
    //(If he tucks his legs up by 40 units, slide the mesh down by 40 units)
    TargetMeshZ = OriginalMeshZ - 40.f;

    //enable tick for interpolation:
    SetActorTickEnabled(true);

    bIsSlamJumping = true;
	LaunchCharacter(CalculatedVelocity, true, true);
}

void ARodmasterEnemy::PauseSlamMontage()
{
    if(!bIsSlamJumping) return; //if landed early then no need to pause

    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    if(AnimInstance && CloseSlamMontage)
    {
        AnimInstance->Montage_SetPlayRate(CloseSlamMontage, 0.0f);
    }
}

void ARodmasterEnemy::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);

    if(bIsSlamJumping)
    {
        bIsSlamJumping = false;

        TargetMeshZ = OriginalMeshZ;

        bIsLandingRecovery = true;

        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        if(AnimInstance && CloseSlamMontage)
        {
            AnimInstance->Montage_SetPlayRate(CloseSlamMontage, 1.0f);
        }
    }
}

void ARodmasterEnemy::ExecuteOverchargeExplosion()
{
    if(bIsDead) return;
    TriggerExplosion(EExplosionType::Overcharge);
}

void ARodmasterEnemy::UpdateOverchargeVisuals(float OverchargeRatio)
{
    if(DynamicGlowMat)
	{
		float CurrentGlow = OverchargeRatio * MaxGlowIntensity;
		DynamicGlowMat->SetScalarParameterValue(FName("GlowIntensity"), CurrentGlow);

        //if setup GlowColor parameter in material(for dynamic color change):
        FLinearColor SafeColor = FLinearColor(0.0f, 1.0f, 0.0f); // green
		FLinearColor DangerColor = FLinearColor(1.0f, 0.0f, 0.0f); // Pure Red
		
		FLinearColor CurrentColor = FMath::Lerp(SafeColor, DangerColor, OverchargeRatio);
		DynamicGlowMat->SetVectorParameterValue(FName("GlowColor"), CurrentColor);
	}
}

void ARodmasterEnemy::StartOverchargeSequence()
{
    if(bIsExploding) return;

    bIsExploding = true;

    //force AI and movement to stop:
    GetCharacterMovement()->DisableMovement();
    if(AEnemyAIController* AICon = Cast<AEnemyAIController>(GetController())) AICon->SleepAI();

    //interrupt any anim montages and play overcharge one:
    if(GetMesh()->GetAnimInstance())
    {
        GetMesh()->GetAnimInstance()->Montage_Stop(0.1f, nullptr); 

        if(OverchargeExplosionMontages.Num() > 0)
        {
            UAnimMontage* MontageToPlay = OverchargeExplosionMontages[FMath::RandRange(0, OverchargeExplosionMontages.Num() - 1)];
            if(MontageToPlay) GetMesh()->GetAnimInstance()->Montage_Play(MontageToPlay, 1.f);
        }
        else ExecuteOverchargeExplosion(); //failsafe: if no montage provided.
    }
}
