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

// Sets default values
AHordeShooterEnemy::AHordeShooterEnemy()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

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

	if(GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->OnMontageEnded.AddDynamic(this, &AHordeShooterEnemy::OnMontageEnded);
	}
	
	DeactivateEnemy(); //start inactive.
}

void AHordeShooterEnemy::ActivateEnemy(const FTransform& SpawnTransform)
{
	bIsActive = true;
	bIsDead = false;
	bIsAttacking = false;
	bIsStunned = false;
	CurrentHealth = MaxHealth;

	//reset physics and collision first. (order matters)
	GetMesh()->SetSimulatePhysics(false);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	//force the mesh back to standing collision profile
	GetMesh()->SetCollisionProfileName(TEXT("CharacterMesh"));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	//reattach mesh to capsule
	GetMesh()->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
	GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
	GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));

	//reset the capusle rotation i.e. we take only the yaw, the direction in which spawnpoint arrow points. 
	FRotator SafeRotation = FRotator(0.0f, SpawnTransform.Rotator().Yaw, 0.0f);
	SetActorRotation(SafeRotation, ETeleportType::TeleportPhysics);

	//teleport to spawnpoint.
	FVector SafeLocation = SpawnTransform.GetLocation() + FVector(0.0f, 0.0f, 50.0f);
	SetActorLocation(SafeLocation, false, nullptr, ETeleportType::TeleportPhysics);

	//wakeup visuals and anims.
	SetActorHiddenInGame(false);
	GetMesh()->bPauseAnims = false;

	//wakeup audio comp
	if (SprintAudioComp && SprintAudioComp->Sound) SprintAudioComp->Play();

	//wakeup the enemy AI.
	if(AEnemyAIController* AICon = Cast<AEnemyAIController>(GetController())) AICon->WakeAI();
}

void AHordeShooterEnemy::DeactivateEnemy()
{
	bIsActive = false;

	//physics off
	GetMesh()->SetSimulatePhysics(false);
	GetMesh()->bPauseAnims = true;

	//collision off
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionProfileName(TEXT("NoCollision")); // Extra safety net

	//hide
	SetActorHiddenInGame(true);
	SetActorLocation(FVector(0.0f, 0.0f, -10000.0f), false, nullptr, ETeleportType::TeleportPhysics);

	//stop audio comp
	if(SprintAudioComp) SprintAudioComp->Stop();
}

//COMBAT ACTIONS:
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
						FVector PushDirection = GetActorForwardVector() * 1000.0f; 
						DamageableActor->ReactToHit(AttackDamage, PushDirection, NAME_None);
						
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


bool AHordeShooterEnemy::ReactToHit(float DamageAmount, const FVector& HitImpulse, FName HitBoneName)
{
	if(bIsDead)
	{
		if (GetMesh()->IsSimulatingPhysics())
		{
			GetMesh()->AddImpulse(HitImpulse*0.2f, HitBoneName, true);
		}
		
		return false; //already dead, no crit hit/headshot.
	}

	float FinalDamage = DamageAmount;
	bool bIsHeadshot = false;

	if(BoneDamageMultipliers.Contains(HitBoneName))
	{
		float Multiplier = BoneDamageMultipliers[HitBoneName];
		FinalDamage *= Multiplier;

		if(Multiplier > 1.0f && GEngine)
		{
			if((HitBoneName == FName("Head")))
			{
				bIsHeadshot = true;
			}
			
		}
	}
	
	CurrentHealth -= FinalDamage;
	LastHitImpulse = HitImpulse;
	LastHitBoneName = HitBoneName;
	
	OnHit(FinalDamage); // Triggers Blueprint logic, then C++ default

	if(CurrentHealth <= 0.f)
	{
		Die();
	}
	else
	{
		PlayHitReaction();
	}

	return bIsHeadshot;
}

void AHordeShooterEnemy::Die()
{
	if(bIsDead) return;
	bIsDead = true;

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
	// 1. Disable Capsule collision so players can walk over the corpse
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 2. Enable Ragdoll
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));
	GetMesh()->SetSimulatePhysics(true);

	GetMesh()->bPauseAnims = true;

	GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	//so that ragdoll doesnt ignore bullets.
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	//Add directional shot impulse!
	GetMesh()->AddImpulse(LastHitImpulse, LastHitBoneName, true);
}

