// Fill out your copyright notice in the Description page of Project Settings.


#include "BlitzkriegerEnemy.h"
#include "ArenaManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NiagaraComponent.h"

ABlitzkriegerEnemy::ABlitzkriegerEnemy()
{
	//set custom stats
	MaxHealth = 100.0f;
	AttackRange = 10000.0f;
	FleeRange = 1500.0f;

    bAlwaysFacePlayer = true; //use the 4-way strafing blendSpace
	bStopToAttack = true;
	bStrafeDuringCooldown = true;

	AttackCooldown = 4.0f;
	
	WalkSpeed = 600.0f;
	SprintSpeed = 1200.0f;

	GetCharacterMovement()->RotationRate = FRotator(0.0f, 200.0f, 0.0f);

	LeftEyeGlow = CreateDefaultSubobject<UNiagaraComponent>(TEXT("LeftEyeGlow"));
	LeftEyeGlow->SetupAttachment(GetMesh(), FName("LeftEye")); 

	RightEyeGlow = CreateDefaultSubobject<UNiagaraComponent>(TEXT("RightEyeGlow"));
	RightEyeGlow->SetupAttachment(GetMesh(), FName("RightEye"));
}

void ABlitzkriegerEnemy::BeginPlay()
{
	Super::BeginPlay();

	SetEyeIntensityAndSize(NormalEyeIntensity, NormalEyeSize);
	OnAttackFinished.AddDynamic(this, &ABlitzkriegerEnemy::OnSpellCastFinished);
}

void ABlitzkriegerEnemy::SetEyeIntensityAndSize(float Intensity, float Size)
{
	if(LeftEyeGlow)
	{
		LeftEyeGlow->SetFloatParameter(FName("EyeIntensity"), Intensity);
		LeftEyeGlow->SetFloatParameter(FName("EyeSize"), Size);
	}
	
	if(RightEyeGlow) 
	{
		RightEyeGlow->SetFloatParameter(FName("EyeIntensity"), Intensity);
		RightEyeGlow->SetFloatParameter(FName("EyeSize"), Size);
	}
}

void ABlitzkriegerEnemy::PerformMeleeAttack()
{
    Super::PerformMeleeAttack();
	SetEyeIntensityAndSize(AttackEyeIntensity, AttackEyeSize);
}

void ABlitzkriegerEnemy::CastLightningOnPlayer()
{
    if(bIsDead || bIsStunned) return;

    AActor* ArenaActor = UGameplayStatics::GetActorOfClass(GetWorld(), AArenaManager::StaticClass());

    if(ArenaActor)
	{
		AArenaManager* Arena = Cast<AArenaManager>(ArenaActor);
		Arena->StartLightningWave(NumberOfLightningsInAWave);
	}
}

void ABlitzkriegerEnemy::OnSpellCastFinished()
{
	SetEyeIntensityAndSize(NormalEyeIntensity, NormalEyeSize);
}