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
    
    //Rotation Handling:
    //if its a strafer(4-way movement anims), stay locked on to the player.
    //if its a rusher(1-way movement anim), rotation is oriented to movement.
    if(ControlledEnemy->bAlwaysFacePlayer)
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

    //STATE: Fleeing
    if(ControlledEnemy->FleeRange > 0.f && DistSquared < FleeRangeSq)
    {
        CurrentState = EAIState::Fleeing;

        //move away from player
        FVector RunDirection = (ControlledEnemy->GetActorLocation() - PlayerTarget->GetActorLocation()).GetSafeNormal2D();
        FVector FleeLocation =  ControlledEnemy->GetActorLocation() + (RunDirection * 1000.f);

        MoveToLocation(FleeLocation, 50.0f);
    }
    
    //STATE: Attacking
    else if(DistSquared <= AttackRangeSq && CheckLineOfSight())
    {
        CurrentState = EAIState::Attacking;

        if(ControlledEnemy->bStopToAttack)
		{
			StopMovement(); //halt to play attack anim
		}
		else
		{
			MoveToPlayer(); //play attack anim while moving
		}

        //this must be true for every enemy type while attacking
        ControlledEnemy->GetCharacterMovement()->bOrientRotationToMovement = false;
        ControlledEnemy->GetCharacterMovement()->bUseControllerDesiredRotation = true;
        SetFocus(PlayerTarget); 

        ControlledEnemy->PerformMeleeAttack(); //overridable in child class.
    }

    //STATE: Chasing
    else
    {
        CurrentState = EAIState::Chasing;

        //optimization:
        float PlayerDistMovedSq = FVector::DistSquared(PlayerTarget->GetActorLocation(), LastKnownPlayerLocation);
        if(PlayerDistMovedSq > PathUpdateThresholdSquared)
        {
            MoveToPlayer();
            LastKnownPlayerLocation = PlayerTarget->GetActorLocation(); //cache the new location
        }
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

