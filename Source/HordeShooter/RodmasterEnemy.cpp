#include "RodmasterEnemy.h"
#include "RodmasterEnemy.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "EnemyAIController.h"
#include "Components/CapsuleComponent.h"
#include "HordeWaveManager.h"
#include "Components/AudioComponent.h"

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

    OverchargeBuildUpVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("OverchargeBuildUpVFX"));
	OverchargeBuildUpVFX->SetupAttachment(GetMesh(), FName("Spine2")); 
	OverchargeBuildUpVFX->bAutoActivate = false;

    OverchargeBuildUpSFX = CreateDefaultSubobject<UAudioComponent>(TEXT("OverchargeBuildUpSFX"));
	OverchargeBuildUpSFX->SetupAttachment(GetMesh(), FName("Spine2"));
	OverchargeBuildUpSFX->bAutoActivate = false;
}

void ARodmasterEnemy::BeginPlay()
{
    Super::BeginPlay();
    BaseWalkSpeed = WalkSpeed;
    BaseAttackCooldown = AttackCooldown;
    CurrentCloseSlamDamage = DefaultCloseSlamDamage;
    CurrentProjectileDamage = DefaultProjectileDamage;
    CurrentProjectileSpeed = DefaultProjectileSpeed;

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
    float NewZ = FMath::FInterpTo(CurrentLoc.Z, TargetMeshZ, DeltaTime, 5.0f);

    GetMesh()->SetRelativeLocation(
        FVector(CurrentLoc.X, CurrentLoc.Y, NewZ), 
        false,
        nullptr,
        ETeleportType::TeleportPhysics
    );

    if (GEngine) {
        GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, FString::Printf(TEXT("Mesh Z: %f | Target Z: %f"), NewZ, TargetMeshZ));
    }

    if(bIsLandingRecovery && FMath::IsNearlyEqual(NewZ, OriginalMeshZ, 0.5f))
    {
        bIsLandingRecovery = false; //turn off recovery flag
        SetActorTickEnabled(false); //turn off tick
    }
}

//for Difficulty multipliers index 0 is Attack multplier, index 1 is Health multiplier.
void ARodmasterEnemy::ActivateEnemy(const FTransform& SpawnTransform, const TArray<float>& DifficultyMultipliers)
{
    Super::ActivateEnemy(SpawnTransform, DifficultyMultipliers);

    CachedDifficultyAttackMult = DifficultyMultipliers[0];

    CurrentCloseSlamDamage = DefaultCloseSlamDamage * CachedDifficultyAttackMult;
	CurrentProjectileDamage = DefaultProjectileDamage * CachedDifficultyAttackMult;
    CurrentProjectileSpeed = DefaultProjectileSpeed;

    AttackCooldown = BaseAttackCooldown;
    CurrentOvercharge = 0.0f;
	bIsExploding = false;
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
	UpdateOverchargeVisuals(0.0f);//reset glow
}

bool ARodmasterEnemy::ReactToHit(float DamageAmount, const FVector &HitImpulse, FName HitBoneName, FName DamageSource)
{
    if(bIsDead || bIsExploding) return false;

    //if energy based, then overcharge
    if(DamageSource == FName("RayGun") || DamageSource == FName("RayGunAlt") || DamageSource == FName("Lightning") || DamageSource == FName("Plasma"))
    {
        CurrentOvercharge += DamageAmount;
        float ChargeRatio = FMath::Clamp(CurrentOvercharge/MaxOvercharge, 0.f, 1.f);

        //scale speed:
        GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed * FMath::Lerp(1.f, MaxSpeedMultiplier, ChargeRatio);
        CurrentProjectileSpeed = FMath::Lerp(DefaultProjectileSpeed, MaxProjectileSpeed, ChargeRatio);

        //scale attack damage:
        float OverchargeAttackMult = FMath::Lerp(1.f, MaxDamageMultiplier, ChargeRatio);
        float TotalAttackMult = CachedDifficultyAttackMult * OverchargeAttackMult;
        AttackDamage = BaseAttackDamage * TotalAttackMult; //Base attack(from parent class)
		CurrentCloseSlamDamage = DefaultCloseSlamDamage * TotalAttackMult;
		CurrentProjectileDamage = DefaultProjectileDamage * TotalAttackMult;

        AttackCooldown = FMath::Lerp(BaseAttackCooldown, MinAttackCooldown, ChargeRatio);

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
		ResistedDamage *= BulletDamageMultiplier; // X% reduction
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
        ColShape = FCollisionShape::MakeSphere(CloseSlamDamageRadius);
        QueryParams.AddIgnoredActor(this);
        ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
        ExplosionDamage = CurrentCloseSlamDamage;
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
				Blast->SetFloatParameter(FName("BlastScale"), CloseSlamDamageRadius*2); 
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

        if(GetMesh()) ExplodeLoc = GetMesh()->GetSocketLocation(FName("Spine2"));
        if(OverchargeExplosionSFX) UGameplayStatics::PlaySoundAtLocation(GetWorld(), OverchargeExplosionSFX, ExplodeLoc);
        if(OverchargeExplosionVFX) UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), OverchargeExplosionVFX, ExplodeLoc);
        if(OverchargeExpCameraShake)
        {
            UGameplayStatics::PlayWorldCameraShake(
                GetWorld(),
                OverchargeExpCameraShake,
                ExplodeLoc,
                OverchargeExpShakeInnerRadius,
                OverchargeExpShakeOuterRadius,
                1.f,
                false  
            );
        }

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
    TargetMeshZ = OriginalMeshZ - 12.0f;

    //enable tick for interpolation:
    SetActorTickEnabled(true);

    bIsSlamJumping = true;
	LaunchCharacter(CalculatedVelocity, true, true);
}

void ARodmasterEnemy::ExecuteSlam()
{
    if(bIsDead || bIsStunned || bIsExploding) return;
    TriggerExplosion(EExplosionType::Slam);
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

        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        if(AnimInstance && CloseSlamMontage)
        {
            AnimInstance->Montage_SetPlayRate(CloseSlamMontage, 1.0f);
        }
    }
}

void ARodmasterEnemy::LandingRecovery()
{
    TargetMeshZ = OriginalMeshZ;
    bIsLandingRecovery = true;
    SetActorTickEnabled(true);
}

void ARodmasterEnemy::PlayRateFWD()
{
    GetMesh()->GetAnimInstance()->Montage_SetPlayRate(OverchargeExplosionMontage, 1.0f);
}

void ARodmasterEnemy::PlayRateBKD()
{
    GetMesh()->GetAnimInstance()->Montage_SetPlayRate(OverchargeExplosionMontage, -1.0f);
}

void ARodmasterEnemy::ExecuteOverchargeExplosion()
{
    if(bIsDead) return;
    if(OverchargeBuildUpVFX) OverchargeBuildUpVFX->DeactivateImmediate();
    if(OverchargeBuildUpSFX) OverchargeBuildUpSFX->Stop();

    TriggerExplosion(EExplosionType::Overcharge);
}

void ARodmasterEnemy::UpdateOverchargeVisuals(float OverchargeRatio)
{
    if(DynamicGlowMat)
	{
		float CurrentGlow = OverchargeRatio * MaxGlowIntensity;
		DynamicGlowMat->SetScalarParameterValue(FName("GlowIntensity"), CurrentGlow);

        //if setup GlowColor parameter in material(for dynamic color change):
        FLinearColor SafeColor = FLinearColor(0.0f, 0.0f, 1.0f); // blue
		FLinearColor DangerColor = FLinearColor(1.0f, 0.0f, 0.0f); //Red
		
		CurrentColor = FMath::Lerp(SafeColor, DangerColor, OverchargeRatio);
		DynamicGlowMat->SetVectorParameterValue(FName("GlowColor"), CurrentColor);
	}
}

void ARodmasterEnemy::StartOverchargeSequence()
{
    if(bIsExploding) return;

    bIsExploding = true;

    //force AI and movement to stop:
    GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->MaxWalkSpeed = 0.0f;

    if(AEnemyAIController* AICon = Cast<AEnemyAIController>(GetController())) AICon->SleepAI();

    //interrupt any anim montages and play overcharge one:
    if(GetMesh()->GetAnimInstance())
    {
        GetMesh()->GetAnimInstance()->Montage_Stop(0.1f, nullptr); 
    
        if(OverchargeExplosionMontage) GetMesh()->GetAnimInstance()->Montage_Play(OverchargeExplosionMontage, 1.f);

        if(OverchargeBuildUpVFX)
        {
            OverchargeBuildUpVFX->SetFloatParameter(FName("BuildUpTime"), OverchargeBuildUpTime);
            OverchargeBuildUpVFX->Activate(true);
        }

        if(OverchargeBuildUpSFX)
        {
            OverchargeBuildUpSFX->Play();
        }
        
        GetWorldTimerManager().SetTimer(OverchargeDetonationTimer, this, &ARodmasterEnemy::ExecuteOverchargeExplosion, OverchargeBuildUpTime, false);
    }
}

void ARodmasterEnemy::ExecutePlasmaShot()
{
	if(bIsDead || bIsStunned || bIsExploding) return;

	APawn* PlayerTarget = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if(!PlayerTarget) return;

	FVector StartLoc = GetMesh()->GetSocketLocation(FName("Projectile_Socket"));
	FVector TargetLoc = PlayerTarget->GetActorLocation(); 
	FVector BaseAimDirection  = (TargetLoc - StartLoc).GetSafeNormal();
    BaseAimDirection.Z = 0;

	AActor* WaveManagerActor = UGameplayStatics::GetActorOfClass(GetWorld(), AHordeWaveManager::StaticClass());
	if(AHordeWaveManager* WaveManager = Cast<AHordeWaveManager>(WaveManagerActor))
	{
        TArray<float> SpreadAngles = {-10.f, 0.f, 10.f};

        for(float Angle : SpreadAngles)
        {
            //rotate the base aim direction in given angles, around Z axis.
            FVector FireDirection  = BaseAimDirection.RotateAngleAxis(Angle, FVector::UpVector);
            WaveManager->SpawnEnemyProjectile(StartLoc, FireDirection, this, CurrentColor, CurrentProjectileDamage, CurrentProjectileSpeed);
        }
	}
}


void ARodmasterEnemy::OnDeath_Implementation()
{
	if(LastDamageSource == FName("Overcharge"))
	{
		GetCharacterMovement()->DisableMovement();
		GetCharacterMovement()->StopMovementImmediately();
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		
		GetMesh()->SetSimulatePhysics(false);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GetMesh()->bPauseAnims = true;

		SetActorHiddenInGame(true);

		if(GibbingExplosionVFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), GibbingExplosionVFX, GetActorLocation(), GetActorRotation());
		}
	}
	else
	{
		Super::OnDeath_Implementation();
	}
}