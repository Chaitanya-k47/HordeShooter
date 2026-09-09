// Fill out your copyright notice in the Description page of Project Settings.


#include "HordeShooterEnemy.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "HordeShooterCharacter.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "EnemyAIController.h"
#include "Components/AudioComponent.h"
#include "HordeShooterPlayerController.h"
#include "HordeWaveManager.h"

// Sets default values
AHordeShooterEnemy::AHordeShooterEnemy()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
	GetMesh()->SetRenderCustomDepth(true);

	//let the AI Controller handle rotation, not the actor itself
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->bOrientRotationToMovement = true;
	MoveComp->bUseControllerDesiredRotation = false;
	MoveComp->RotationRate = FRotator(0.f, 600.f, 0.f);

	SprintAudioComp = CreateDefaultSubobject<UAudioComponent>(TEXT("SprintAudioComp"));
	SprintAudioComp->SetupAttachment(GetMesh(), FName("Head"));
	SprintAudioComp->bAutoActivate = false;

	//CPU OPTIMIZATIONS:
	MoveComp->bSweepWhileNavWalking = false;
	MoveComp->bUseRVOAvoidance = true; 
	MoveComp->AvoidanceConsiderationRadius = 50.f;
	MoveComp->bEnablePhysicsInteraction = false;
	GetMesh()->bEnableUpdateRateOptimizations = true;
	GetCapsuleComponent()->PrimaryComponentTick.bCanEverTick = false;
	GetCapsuleComponent()->SetCanEverAffectNavigation(false);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);
	GetMesh()->SetGenerateOverlapEvents(false);
	bReplicates = false;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bCanEverTick = false;

	//anim culling:
	/*
		AlwaysTickPoseAndRefreshBones (default), (expensive)
		OnlyTickPoseWhenRendered (enemy looses its ability to cause damage to the player (when not rendered on screen) if attacks are bound to anim notifs.)
		AlwaysTickPose (evaluates the animation logic(anim notifs) but skips drawing and stretching the 3D mesh bones)
	*/
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
}

// Called when the game starts or when spawned
void AHordeShooterEnemy::BeginPlay()
{
	Super::BeginPlay();

	BaseMaxHealth = MaxHealth;
	BaseAttackDamage = AttackDamage;

	if(GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->OnMontageEnded.AddDynamic(this, &AHordeShooterEnemy::OnMontageEnded);
	}

	AActor* WaveManagerActor = UGameplayStatics::GetActorOfClass(GetWorld(), AHordeWaveManager::StaticClass());
	if(AHordeWaveManager* WaveManager = Cast<AHordeWaveManager>(WaveManagerActor)) CachedWaveManager = WaveManager;
	
	DeactivateEnemy(); //start inactive.
}

//for Difficulty multipliers index 0 is Attack multplier, index 1 is Health multiplier.
void AHordeShooterEnemy::ActivateEnemy(const FTransform& SpawnTransform, const TArray<float>& DifficultyMultipliers)
{
	//apply difficulty scaling:
	AttackDamage = BaseAttackDamage * DifficultyMultipliers[0];
	MaxHealth = BaseMaxHealth * DifficultyMultipliers[1];

	bIsActive = true;
	bIsDead = false;
	bIsAttacking = false;
	bIsStunned = false;
	CurrentHealth = MaxHealth;

	GetMesh()->SetSimulatePhysics(false); //Failsafe

	//reset the capusle rotation i.e. we take only the yaw, the direction in which spawnpoint arrow points. 
	FRotator SafeRotation = FRotator(0.0f, SpawnTransform.Rotator().Yaw, 0.0f);
	SetActorRotation(SafeRotation, ETeleportType::TeleportPhysics);

	//teleport to spawnpoint.
	FVector SafeLocation = SpawnTransform.GetLocation() + FVector(0.0f, 0.0f, 50.0f);
	SetActorLocation(SafeLocation, false, nullptr, ETeleportType::TeleportPhysics);

	//reset physics and collision first. (order matters)
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	//force the mesh back to standing collision profile
	GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	GetMesh()->bPauseAnims = false;
	if(GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Stop(0.0f, nullptr); 
	}

	//wakeup visuals
	SetActorHiddenInGame(false);

	//wakeup audio comp
	if (SprintAudioComp && SprintAudioComp->Sound) SprintAudioComp->Play();

	//wakeup the enemy AI.
	if(AEnemyAIController* AICon = Cast<AEnemyAIController>(GetController())) AICon->WakeAI();
}

void AHordeShooterEnemy::DeactivateEnemy()
{
	bIsActive = false;

	//stop audio comp
	if(SprintAudioComp) SprintAudioComp->Stop();

	//kill physics first
	GetMesh()->PutAllRigidBodiesToSleep();
	GetMesh()->SetSimulatePhysics(false);
	GetMesh()->bPauseAnims = true;

	//collision off
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionProfileName(TEXT("NoCollision")); //extra safety net
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//re-attach the mesh
	GetMesh()->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepWorldTransform);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	//hide
	SetActorHiddenInGame(true);
	SetActorLocation(FVector(0.0f, 0.0f, -10000.0f), false, nullptr, ETeleportType::TeleportPhysics);
}


void AHordeShooterEnemy::CancelAttack()
{
	if(!bIsAttacking) return;

	bIsAttacking = false;

	if(GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Stop(0.2f, nullptr);
	}
}


// COMBAT ACTIONS:
void AHordeShooterEnemy::PerformMeleeAttack()
{
	if(bIsAttacking || bIsStunned || bIsDead || AttackMontages.Num() == 0) return;

	bIsAttacking = true;
	int32 RandomIndex = FMath::RandRange(0, AttackMontages.Num() - 1);

	if(AttackSound)
	{
		UGameplayStatics::SpawnSoundAttached(AttackSound, GetMesh(), FName("Head"));
	}

	if(GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(AttackMontages[RandomIndex]);
	}
}

void AHordeShooterEnemy::ExecuteAttack()
{
	if(bIsDead) return;
	
	//we put the center of the sphere halfway to the max range, and make its radius the other half.
	float StrikeRadius = AttackRange / 2.0f; 
	FVector StrikeLocation = GetActorLocation() + (GetActorForwardVector() * StrikeRadius);

	TArray<FOverlapResult> OverlapResults;
	FCollisionShape SphereCol = FCollisionShape::MakeSphere(StrikeRadius);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->OverlapMultiByChannel(
		OverlapResults,
		StrikeLocation,
		FQuat::Identity,
		ECC_Pawn, 
		SphereCol,
		QueryParams
	);

	//DrawDebugSphere(GetWorld(), StrikeLocation, StrikeRadius, 12, bHit ? FColor::Red : FColor::Green, false, 1.0f);

	if(bHit)
	{
		for(const FOverlapResult& Overlap : OverlapResults)
		{
			AActor* HitActor = Overlap.GetActor();

			if (HitActor && HitActor->IsA(AHordeShooterCharacter::StaticClass())) //check if the hit actor is player
			{
				if (HitActor->GetClass()->ImplementsInterface(UDamageableInterface::StaticClass()))
				{
					IDamageableInterface* DamageableActor = Cast<IDamageableInterface>(HitActor);
					if (DamageableActor)
					{
						//Push the player slightly backward
						FVector PushImpulse = GetActorForwardVector() * 100000.0f; 
						DamageableActor->ReactToHit(AttackDamage, PushImpulse, NAME_None);
						
						break; //we hit the player, no need to keep looping
					}
				}
			}
		}
	}
}


void AHordeShooterEnemy::PlayHitReaction()
{
	if(bIsDead || HitReactionMontages.Num() == 0) return;

	bIsStunned = true;
	int32 RandomIndex = FMath::RandRange(0, HitReactionMontages.Num() - 1);
	
	if(GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(HitReactionMontages[RandomIndex]);
	}
}


void AHordeShooterEnemy::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if(AttackMontages.Contains(Montage))
	{
		bIsAttacking = false;
		OnAttackFinished.Broadcast(); //tell AI controller that the attack is finished.
	}
	else if(HitReactionMontages.Contains(Montage))
	{
		bIsStunned = false;
	}
}


bool AHordeShooterEnemy::ReactToHit(float DamageAmount, const FVector& HitImpulse, FName HitBoneName, FName DamageSource)
{
	if(bIsDead)
	{
		if(GetMesh()->IsSimulatingPhysics())
		{
			//Force mode.
			GetMesh()->AddImpulse(HitImpulse, HitBoneName, false);
		}
		
		return false; //already dead, no crit hit/headshot.
	}

	float FinalDamage = DamageAmount;
	bool bIsHeadshot = false;
	FVector NewHitImpulse = HitImpulse;

	if(BoneDamageMultipliers.Contains(HitBoneName))
	{
		float Multiplier = BoneDamageMultipliers[HitBoneName];
		FinalDamage *= Multiplier;
		NewHitImpulse = HitImpulse * Multiplier;

		if(Multiplier > 1.0f)
		{
			if((HitBoneName == FName("Head")))
			{
				bIsHeadshot = true;
			}
		}
	}
	
	CurrentHealth -= FinalDamage;
	LastHitImpulse = NewHitImpulse;
	LastHitBoneName = HitBoneName;
	LastDamageSource = DamageSource;
	
	OnHit(FinalDamage); //triggers Blueprint logic, then C++ default

	if(CurrentHealth <= 0.f)
	{
		Die();
	}
	else
	{
		PlayHitReaction();

		//velocity mode by default but we trick it into Force mode by Dividing impulse(momentum vector) by mass to get real velocity.
		float EnemyMass = GetCharacterMovement()->Mass;
		if(EnemyMass <= 0.0f) EnemyMass = 100.0f; //fail safe
		FVector CalculatedVelocity = LastHitImpulse/EnemyMass;
		LaunchCharacter(CalculatedVelocity, true, true);
	}

	return bIsHeadshot;
}

void AHordeShooterEnemy::Die()
{
	if(bIsDead) return;

	//report the kill to announcer:
	if(APlayerController* BasePC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		if(AHordeShooterPlayerController* PC = Cast<AHordeShooterPlayerController>(BasePC))
		{
			bool bWasHeadshot = (LastHitBoneName == FName("Head") && LastDamageSource != FName("Melee"));
			bool bWasSlam = (LastHitBoneName == FName("Slam") || LastDamageSource == FName("Slam"));

			PC->AddKill(bWasHeadshot, bWasSlam); 
		}
	}

	bIsDead = true;

	//Drop logic:
	if(CachedWaveManager && AmmoDrops.Num() > 0)
	{
		//ammo drop (RNG or HEADSHOT):
		EPickupSize AmmoSizeToDrop = EPickupSize::Small;
		bool bShouldDropAmmo = false;
		bool bWasHeadshot = (LastHitBoneName == FName("Head"));

		if(bWasHeadshot)
		{	
			//guaranteed drop on headshot, default to the first configured drop
			bShouldDropAmmo = true;
			AmmoSizeToDrop = AmmoDrops[0].Size; 
		}
		else
		{
			float Roll = FMath::FRand();
			float CumulativeChance = 0.0f;
			
			for(const FAmmoDropConfig& Drop : AmmoDrops)
			{
				CumulativeChance += Drop.DropChance;
				if(Roll <= CumulativeChance)
				{
					bShouldDropAmmo = true;
					AmmoSizeToDrop = Drop.Size;
					break;
				}
			}
		}

		if(bShouldDropAmmo) CachedWaveManager->SpawnPickup(GetActorLocation(), EPickupType::Ammo, AmmoSizeToDrop);

		//Health drop (MELEE or PROXIMITY):
		EPickupSize HealthSizeToDrop = EPickupSize::Small;
		bool bShouldDropHealth = false;

		if(LastDamageSource == FName("Melee"))
		{
			bShouldDropHealth = true;
			HealthSizeToDrop = HealthDropSettings.MeleeDropSize;
		}
		else
		{
			//check proximity:
			APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
			if(PC && PC->GetPawn())
			{
				float DistSq = FVector::DistSquared(GetActorLocation(), PC->GetPawn()->GetActorLocation());
				float RadiusSq = FMath::Square(HealthDropSettings.ProximityRadius);

				if(DistSq <= RadiusSq)
				{
					bShouldDropHealth = true;
					HealthSizeToDrop = HealthDropSettings.ProximityDropSize;
				}
			}
		}

		if(bShouldDropHealth) CachedWaveManager->SpawnPickup(GetActorLocation(), EPickupType::Health, HealthSizeToDrop);

		//Surge drop:
		/*
			implementation
		*/
	}

	if(SprintAudioComp)
	{
		SprintAudioComp->Stop();
	}

	OnEnemyKilled.Broadcast(); //tell wave manager this enemy is dead
	
	OnDeath(); // Triggers BP logic, then C++ default

	//Put Enemy AI to sleep and set timer for it to Deactivate/Despawn.
	if(AEnemyAIController* AICon = Cast<AEnemyAIController>(GetController())) AICon->SleepAI();
	GetWorldTimerManager().SetTimer(DespawnTimerHandle, this, &AHordeShooterEnemy::DeactivateEnemy, 5.0f, false);	
}

void AHordeShooterEnemy::OnHit_Implementation(float DamageAmount)
{
	
}

void AHordeShooterEnemy::OnDeath_Implementation()
{
	//stop applying gravity, walking, or falling to the capsule.
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	//Enable Ragdoll
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->bPauseAnims = true;

	//detach mesh:
	GetMesh()->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

	//mesh ignores living pawns but react to bullets
	GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	//add impulse (Force mode.)
	FName PhysicsBone = LastHitBoneName;
	if (PhysicsBone == FName("Slam"))
	{
		// "Slam" isn't a real bone, reset it to NAME_None 
		PhysicsBone = NAME_None; 
	}
	GetMesh()->AddImpulse(LastHitImpulse, PhysicsBone, false);
}

