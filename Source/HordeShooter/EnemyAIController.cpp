// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAIController.h"

#include "HordeShooterEnemy.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"

AEnemyAIController::AEnemyAIController()
{
    
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    ControlledEnemy = Cast<AHordeShooterEnemy>(InPawn);

    if(ControlledEnemy)
    {
        ControlledEnemy->OnAttackFinished.AddDynamic(this, &AEnemyAIController::OnEnemyAttackFinished);
        
        //wait a short time before trying to find the player, to ensure the player has spawned
        GetWorldTimerManager().SetTimer(FindPlayerTimer, this, &AEnemyAIController::FindPlayer, 0.1f, true);
    }
}

void AEnemyAIController::FindPlayer()
{
	PlayerTarget = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	
	if(PlayerTarget)
	{
        GetWorldTimerManager().ClearTimer(FindPlayerTimer);//player found stop the timer.
        CurrentState = EAIState::Chasing;

        //start the AI tick timer to update AI logic every 0.1 seconds
        //OPTIMIZATION: Staggered AI updates, this forces multiple enemies to not update their logic at the same time but distribute evenly among frames,
        //hence not causing cpu spikes.
        float RandomStartDelay = FMath::RandRange(0.0f, 0.1f);

        GetWorldTimerManager().SetTimer(AITickTimer, this, &AEnemyAIController::UpdateAILogic, 0.1f, true, RandomStartDelay);
	}
}

void AEnemyAIController::UpdateAILogic()
{
    if (!ControlledEnemy || !PlayerTarget || ControlledEnemy->bIsDead) 
	{
		StopMovement();
		return;
	}

    //OPTIMIZATION: Using Distance Squared intead of Dist.
    float DistSquared = FVector::DistSquared(ControlledEnemy->GetActorLocation(), PlayerTarget->GetActorLocation());
    float AttackRangeSq = FMath::Square(ControlledEnemy->AttackRange); //sq the attack range for comparison.
    float FleeRangeSq = FMath::Square(ControlledEnemy->FleeRange);

    // GEngine->AddOnScreenDebugMessage(-1, 0.2f, FColor::Green, FString::Printf(TEXT("Distance to Player: %f"), DistanceToPlayer));
    // GEngine->AddOnScreenDebugMessage(-1, 0.2f, FColor::Green, FString::Printf(TEXT("AI State: %s"), *UEnum::GetValueAsString(CurrentState)));

    //STATE: Fleeing
    if(ControlledEnemy->FleeRange > 0.f && DistSquared < FleeRangeSq)
    {
        if(ControlledEnemy->bCancelsAttackInFleeRange && ControlledEnemy->bIsAttacking) ControlledEnemy->CancelAttack();

        CurrentState = EAIState::Fleeing;
        ControlledEnemy->GetCharacterMovement()->MaxWalkSpeed = ControlledEnemy->SprintSpeed;

        UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
        if(NavSys)
        {
            //ideal dir is straight away from player
            FVector IdealRunDir = (ControlledEnemy->GetActorLocation() - PlayerTarget->GetActorLocation()).GetSafeNormal2D();
            FVector BestFleeLoc = ControlledEnemy->GetActorLocation(); //default to stying still if cornered.

            //angles to test
            float AnglesToTry[5] = { 0.0f, 45.0f, -45.0f, 90.0f, -90.0f };

            for(float Angle : AnglesToTry)
            {
                //rotate the run direction:
                FVector TestDir = IdealRunDir.RotateAngleAxis(Angle, FVector::UpVector);
                FVector TestLoc = ControlledEnemy->GetActorLocation() + (TestDir * 1000.0f);

                //check if the spot is on nav mesh:
                FNavLocation NavHit;
				if(NavSys->ProjectPointToNavigation(TestLoc, NavHit, FVector(200.f, 200.f, 200.f)))
				{
					BestFleeLoc = NavHit.Location;
					break;
				}
            }

            MoveToLocation(BestFleeLoc, 50.0f);
        }
    }
    
    //STATE: Attacking
    else if(DistSquared <= AttackRangeSq && CheckLineOfSight())
    {
        if(!bIsOnCooldown && !ControlledEnemy->bIsAttacking) //not attacking and no on cooldown so attack.
        {
            ControlledEnemy->PerformMeleeAttack(); //overridable in child class.
            
            if(ControlledEnemy->AttackCooldown > 0.0f)
			{
				bIsOnCooldown = true;
				GetWorldTimerManager().SetTimer(AttackCooldownTimerHandle, this, &AEnemyAIController::ResetAttackCooldown, ControlledEnemy->AttackCooldown, false);
			}
        }

        if(ControlledEnemy->bIsAttacking)
		{
			CurrentState = EAIState::Attacking;

			if(ControlledEnemy->bStopToAttack) StopMovement();
			else if(ControlledEnemy->bChargeOnPlayerWhileAttacking) MoveToPlayer();
		}
        else if(bIsOnCooldown) //waiting
        {
            if(ControlledEnemy->bStrafeDuringCooldown) //do we strafe?
			{
				CurrentState = EAIState::Idle; 
				ControlledEnemy->GetCharacterMovement()->MaxWalkSpeed = ControlledEnemy->WalkSpeed;

				if(CurrentStrafeTarget.IsNearlyZero() || FVector::DistSquared(ControlledEnemy->GetActorLocation(), CurrentStrafeTarget) < 25000.0f)
				{
					PickNewStrafeTarget();
				}
				MoveToLocation(CurrentStrafeTarget, 50.0f);
			}
			else
			{
				//no strafing allowed
				CurrentState = EAIState::Chasing;
				ControlledEnemy->GetCharacterMovement()->MaxWalkSpeed = ControlledEnemy->SprintSpeed;
				MoveToPlayer();
			}
        }
    }

    //STATE: Chasing
    else
    {
        CurrentState = EAIState::Chasing;
        ControlledEnemy->GetCharacterMovement()->MaxWalkSpeed = ControlledEnemy->SprintSpeed;

        //optimization:
        float PlayerDistMovedSq = FVector::DistSquared(PlayerTarget->GetActorLocation(), LastKnownPlayerLocation);
        if(PlayerDistMovedSq > PathUpdateThresholdSquared)
        {
            MoveToPlayer();
            LastKnownPlayerLocation = PlayerTarget->GetActorLocation(); //cache the new location
        }
    }

    //Rotation Handling:
    //if its a strafer(4-way movement anims), stay locked on to the player.
    //if its a rusher(1-way movement anim), rotation is oriented to movement.
    if(ControlledEnemy->bAlwaysFacePlayer || CurrentState == EAIState::Attacking)
	{
		ControlledEnemy->GetCharacterMovement()->bOrientRotationToMovement = false;
		ControlledEnemy->GetCharacterMovement()->bUseControllerDesiredRotation = true;
		SetFocus(PlayerTarget);
	}
	else
	{
		ControlledEnemy->GetCharacterMovement()->bOrientRotationToMovement = true;
		ControlledEnemy->GetCharacterMovement()->bUseControllerDesiredRotation = false;
		ClearFocus(EAIFocusPriority::Gameplay);
	}

}

bool AEnemyAIController::CheckLineOfSight()
{
	if(!ControlledEnemy || !PlayerTarget) return false;

	FHitResult Hit;
	FVector Start = ControlledEnemy->GetActorLocation();
	FVector End = PlayerTarget->GetActorLocation();
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(ControlledEnemy);

	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Pawn, Params);

	//if we hit the player, or hit nothing at all LOS is clear
	if(bHit && Hit.GetActor() == PlayerTarget) return true;
	if(!bHit) return true; 

	return false;
}

void AEnemyAIController::OnEnemyAttackFinished()
{
	//montage ended! Resume chasing.
	CurrentState = EAIState::Chasing;
	ClearFocus(EAIFocusPriority::Gameplay); //unlock the neck so NavMesh can steer again
}

//MoveToActor() if on nav mesh, if not then dont call MoveToActor() as it conflicts with MoveToLocation() and the enemy slides slowly.
//MoveToPlayer() fn uses this distinction.
void AEnemyAIController::MoveToPlayer()
{
    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if(NavSys)
    {
        //Check if the player is on the nav mesh.
        //logic: if the 50x50x50 box(extent of projection) around the player overlaps with the nave mesh at any point, then bIsPlayerOnNavMesh = true. 
        FNavLocation NavCheck;
        bool bIsPlayerOnNavMesh = NavSys->ProjectPointToNavigation(PlayerTarget->GetActorLocation(), NavCheck, FVector(50.f, 50.f, 50.f));

        if (bIsPlayerOnNavMesh)
        {   
            //call MoveToActor
            MoveToActor(PlayerTarget, 100.f);
            LastEdgeLocation = FVector::ZeroVector; // Reset our edge tracker
        }
        else
        {
            //player is off the mesh
            //donot call MoveToActor use MoveToLocation().
            if (NavSys->ProjectPointToNavigation(PlayerTarget->GetActorLocation(), NavCheck, FVector(2000.f, 2000.f, 2000.f)))
            {
                //only update the path if the player moved significantly along the edge
                if (FVector::Dist(LastEdgeLocation, NavCheck.Location) > 50.0f)
                {
                    LastEdgeLocation = NavCheck.Location;
                    MoveToLocation(LastEdgeLocation, 100.f);
                }
            }
        }
    }
}

void AEnemyAIController::ResetAttackCooldown()
{
    bIsOnCooldown = false;
}

void AEnemyAIController::PickNewStrafeTarget()
{
    if(!ControlledEnemy || !PlayerTarget) return;

    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
    if(NavSys)
    {
        //get dir from player to enemy:
        FVector DirFromPlayer = (ControlledEnemy->GetActorLocation() - PlayerTarget->GetActorLocation()).GetSafeNormal2D();

        //calculate exact right vector using cross prodcut:
        FVector RightVector = FVector::CrossProduct(FVector::UpVector, DirFromPlayer).GetSafeNormal2D();

        //15% chance to randomly change direction just to be unpredictable to the player
		if (FMath::FRand() < 0.15f) CurrentStrafeDir *= -1.0f;

        //calculate strafe offset 600 to 1000 units to the side:
        FVector StrafeOffset = RightVector * CurrentStrafeDir  * 1500.f;
        FVector IdealTargetLoc = ControlledEnemy->GetActorLocation() + StrafeOffset;

        //check if this spot is on the navmesh:
        FNavLocation NavHit;
		if(NavSys->ProjectPointToNavigation(IdealTargetLoc, NavHit, FVector(200.f, 200.f, 200.f)))
		{
			CurrentStrafeTarget = NavHit.Location;
		}
        else //hit the edge of the arena swap directions and go opposit way
        {
            CurrentStrafeDir *= -1.0f;
			StrafeOffset = RightVector * CurrentStrafeDir * 1500.f;
			IdealTargetLoc = ControlledEnemy->GetActorLocation() + StrafeOffset;

            if(NavSys->ProjectPointToNavigation(IdealTargetLoc, NavHit, FVector(200.f, 200.f, 200.f)))
			{
				CurrentStrafeTarget = NavHit.Location;
			}
			else
			{
				//if BOTH left and right are blocked (cornered), back up slightly
				CurrentStrafeTarget = ControlledEnemy->GetActorLocation() + (DirFromPlayer * 400.0f); 
			}
        }
    }
}

void AEnemyAIController::WakeAI()
{
    //wake the AI up, start ticking and chasing the player
    CurrentState = EAIState::Chasing;
    GetWorldTimerManager().SetTimer(AITickTimer, this, &AEnemyAIController::UpdateAILogic, 0.1f, true);
}

void AEnemyAIController::SleepAI()
{
    //stut down the AI
    GetWorldTimerManager().ClearTimer(AITickTimer);
    GetWorldTimerManager().ClearTimer(FindPlayerTimer);
    StopMovement();
    ClearFocus(EAIFocusPriority::Gameplay);
}

void AEnemyAIController::BeginPlay()
{
    Super::BeginPlay();

    // APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    // if(PlayerPawn)
    // {
    //     SetFocus(PlayerPawn);
    // }
}

// void AEnemyAIController::Tick(float DeltaTime)
// {
//     Super::Tick(DeltaTime);

//     if(PlayerPawn)
//     {
//         MoveToActor(PlayerPawn, 150.f);
//     }
// }

