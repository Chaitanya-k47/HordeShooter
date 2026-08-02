// Fill out your copyright notice in the Description page of Project Settings.


#include "HordeShooterCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h"

#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/SceneComponent.h"

#include "HordeShooterWeapon.h"
#include "HordeShooterPlayerController.h"
#include "HordeShooterHUDWidget.h"
#include "Components/AudioComponent.h"
#include "ArenaManager.h"

// Sets default values
AHordeShooterCharacter::AHordeShooterCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	GetMesh()->SetTickGroup(ETickingGroup::TG_PostUpdateWork);

	//Setup first person camera and mesh:
	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	Pivot->SetupAttachment(GetRootComponent());

	ShadowProxyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShadowProxyMesh"));
	ShadowProxyMesh->SetupAttachment(GetRootComponent());
	ShadowProxyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShadowProxyMesh->SetCollisionProfileName(TEXT("NoCollision"));
	ShadowProxyMesh->SetHiddenInGame(true);
	ShadowProxyMesh->bCastHiddenShadow = true; 

	CharacterArms = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterArms"));
	CharacterArms->SetupAttachment(Pivot);
	CharacterArms->SetCastShadow(false);

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(CharacterArms, FName("head")); //attach camera to Head bone.
	FirstPersonCamera->bUsePawnControlRotation = false;

	SlideAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("SlideAudioComponent"));
	SlideAudioComponent->SetupAttachment(GetRootComponent());
	SlideAudioComponent->bAutoActivate = false;

	//Movement config:
	GetCharacterMovement()->MaxWalkSpeed = 1500.f; // Base run speed (up from 600)
	GetCharacterMovement()->MaxAcceleration = 4000.f; // Snappy start
	GetCharacterMovement()->BrakingDecelerationWalking = 4000.f; // Snappy stop (no ice skating)
	GetCharacterMovement()->GroundFriction = 8.0f;
	GetCharacterMovement()->SetWalkableFloorAngle(55.f);

	GetCharacterMovement()->GravityScale = 2.f;
	GetCharacterMovement()->JumpZVelocity = 1000.f;
	GetCharacterMovement()->AirControl = 0.8f;
	GetCharacterMovement()->FallingLateralFriction = 3.0f;

}

// Called when the game starts or when spawned
void AHordeShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	CurrentHealth = MaxHealth;
	
	//initialize target FOV to base FOV
	TargetFOV = IdleFOV;

	if(FirstPersonCamera)
	{
		BaseCameraLocation = FirstPersonCamera->GetRelativeLocation();
		BaseCameraRotation = FirstPersonCamera->GetRelativeRotation();
	}

	//initialize capsule half height values for sliding
	DefaultHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
	CrouchedHalfHeight = GetCharacterMovement()->GetCrouchedHalfHeight();
	TargetHalfHeight = DefaultHalfHeight;

	//initialize dash variables
	AvailableDashes = MaxDashes;
	AirDashesUsed = 0;

	//add input mapping context
	APlayerController* PlayerController = Cast<APlayerController>(Controller);
	if(PlayerController)
	{
		ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
		UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);
		if(Subsystem)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	//spawn weapons and add to inventory:
	for(const TSubclassOf<AHordeShooterWeapon>& WeaponClass : DefaultWeaponClasses)
	{
		if(!WeaponClass) continue;

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;

		AHordeShooterWeapon* SpawnedWeapon = GetWorld()->SpawnActor<AHordeShooterWeapon>(WeaponClass, SpawnParams);

		int32 Index = Inventory.Add(SpawnedWeapon);
		SpawnedWeapon->CurrentOwner = this;

		if(Index == CurrentWeaponIndex)
		{
			//equip weapon.
			EquipWeapon(SpawnedWeapon);
		}
	}

	//wait 0.1 seconds to guarantee the HUD has been created by the playercontroller
	FTimerHandle HUDInitTimer;
	GetWorldTimerManager().SetTimer(HUDInitTimer, [this]()
	{
		if(AHordeShooterPlayerController* PC = Cast<AHordeShooterPlayerController>(GetController()))
		{
			if(PC->PlayerHUDWidget)	PC->PlayerHUDWidget->UpdateHealth(CurrentHealth, MaxHealth);
		}

		//spawn weapons and add to inventory:
		for(const TSubclassOf<AHordeShooterWeapon>& WeaponClass : DefaultWeaponClasses)
		{
			if(!WeaponClass) continue;

			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = this;

			AHordeShooterWeapon* SpawnedWeapon = GetWorld()->SpawnActor<AHordeShooterWeapon>(WeaponClass, SpawnParams);

			int32 Index = Inventory.Add(SpawnedWeapon);
			SpawnedWeapon->CurrentOwner = this;

			if(Index == CurrentWeaponIndex)
			{
				//equip weapon.
				EquipWeapon(SpawnedWeapon);
			}
		}

	}, 0.1f, false);

}

// Called every frame
void AHordeShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//DEBUG
	if (GEngine)
	{
		FString DashStatus = FString::Printf(TEXT("Available Dashes: %d / %d | Air Dashes Used: %d"), AvailableDashes, MaxDashes, AirDashesUsed);
		GEngine->AddOnScreenDebugMessage(0, 0.0f, FColor::Cyan, DashStatus);

		// Show if a timer is active
		if (GetWorldTimerManager().IsTimerActive(DashTimerHandle))
		{
			float TimeLeft = GetWorldTimerManager().GetTimerRemaining(DashTimerHandle);
			FString TimerStatus = FString::Printf(TEXT("Recharging in: %.2f sec"), TimeLeft);
			GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Orange, TimerStatus);
		}
		else
		{
			GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Green, TEXT("Dashes Fully Charged"));
		}
	}
	//DEBUG

	Pivot->SetRelativeRotation(FRotator(GetControlRotation().Pitch, 0.f, 0.f));
	

	//dash traversal logic:
	if(bIsDashing)
	{
		DashTimer -= DeltaTime;

		if(DashTimer > 0.f)
			GetCharacterMovement()->Velocity = CurrentDashDirection*DashSpeed;
		else
		{
			bIsDashing = false;

			//kill dash momentum if in air
			if(GetCharacterMovement()->IsFalling())
			{
				FVector CurrentVelocity  = GetCharacterMovement()->Velocity;
				FVector HorizontalVelocity = FVector(CurrentVelocity.X, CurrentVelocity.Y, 0.f);
				float NormalSpeed = GetCharacterMovement()->MaxWalkSpeed;

				if(HorizontalVelocity.Size() > NormalSpeed)
				{
					HorizontalVelocity = HorizontalVelocity.GetSafeNormal() * NormalSpeed;
				}

				GetCharacterMovement()->Velocity = FVector(HorizontalVelocity.X, HorizontalVelocity.Y, CurrentVelocity.Z);

			}
		}
	}

	//sliding on slope physics:
	if(bIsSliding && GetCharacterMovement()->IsMovingOnGround())
	{
		FVector FloorNormal = GetCharacterMovement()->CurrentFloor.HitResult.ImpactNormal;
		FVector DownhillForce = FVector::VectorPlaneProject(FVector(0.f, 0.f, -1.f), FloorNormal);
		GetCharacterMovement()->Velocity += (DownhillForce * SlideGravityMultiplier * DeltaTime);

		if(GetCharacterMovement()->Velocity.Size2D() > MaxSlideSpeed)
		{
			float CurrentZ = GetCharacterMovement()->Velocity.Z;
			FVector CappedVelocity = GetCharacterMovement()->Velocity.GetSafeNormal2D() * MaxSlideSpeed;
			GetCharacterMovement()->Velocity = FVector(CappedVelocity.X, CappedVelocity.Y, CurrentZ);
		}
			
	}

	//Dynamic FOV state logic:
	if(FirstPersonCamera)
	{		
		// 1. Calculate directional movement alignment
		FVector VelocityDir = GetCharacterMovement()->Velocity.GetSafeNormal();
		FVector ActorForward = GetActorForwardVector();
		FVector ActorRight = GetActorRightVector();
		
		float ForwardAlignment = FVector::DotProduct(VelocityDir, ActorForward);
		float RightAlignment = FVector::DotProduct(VelocityDir, ActorRight);
		float ForwardFactor = FMath::Abs(ForwardAlignment); 

		float CurrentInterpSpeed = RunFOVInterpSpeed;
		float CurrentSpeed = GetVelocity().Size2D();

		FVector TargetCameraLocation = BaseCameraLocation;
		float RollOffset = 0.f;

		//PEDOMETER:
		if (GetCharacterMovement()->IsMovingOnGround() && CurrentSpeed > 10.f && !bIsSliding && !bIsDashing)
		{
			//distance covered this exact frame
			AccumulatedStepDistance += (CurrentSpeed * DeltaTime);

			if (AccumulatedStepDistance >= DistancePerFootstep)
			{
				PlayFootstepSound();
				
				//reset the tracker(but keep any overflow distance for perfect accuracy)
				//AccumulatedStepDistance -= DistancePerFootstep;
				AccumulatedStepDistance = 0.f;
			}
		}
		else if (CurrentSpeed <= 10.f || GetCharacterMovement()->IsFalling())
		{
			//reset pedometer
			AccumulatedStepDistance = 0.f;
		
		}

		// STATE 1: DASHING
		if (bIsDashing)
		{
			TargetFOV = FMath::Lerp(IdleFOV, DashFOV, ForwardFactor);
			CurrentInterpSpeed = DashFOVInterpSpeed;
			
			//this one works the best:
			FVector LagOffset = FVector(0.f, 0.f, -RightAlignment * MaxDashCameraLag);
			TargetCameraLocation = BaseCameraLocation + LagOffset;

			RollOffset = RightAlignment * MaxDashCameraTilt;
		}

		// STATE 2: RUNNING
		else if (CurrentSpeed > 10.f)
		{
			TargetFOV = RunFOV; // Standard run FOV
			CurrentInterpSpeed = RunFOVInterpSpeed;

			RollOffset = RightAlignment * MaxRunCameraTilt;
		}

		// STATE 3: IDLE
		else
		{
			TargetFOV = IdleFOV;
			CurrentInterpSpeed = RunFOVInterpSpeed; 
		}

		// --- APPLY FOV ONLY ---
		float CurrentFOV = FirstPersonCamera->FieldOfView;
		float NewFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, CurrentInterpSpeed);
		FirstPersonCamera->SetFieldOfView(NewFOV);

		// --- APPLY CAMERA TRANSLATION LAG WHEN DASHING ---
		FVector CurrentCameraLocation = FirstPersonCamera->GetRelativeLocation();
		FVector NewCameraLocation = FMath::VInterpTo(CurrentCameraLocation, TargetCameraLocation, DeltaTime, CameraLagInterpSpeed);
		FirstPersonCamera->SetRelativeLocation(NewCameraLocation);

		// --- APPLY CAMERA TILT FOR SIDEWAYS MOVEMENT ---
		FRotator CurrentCameraRotation = FirstPersonCamera->GetRelativeRotation();
		float AbsoluteTargetRoll = BaseCameraRotation.Roll + RollOffset;
		float NewRoll = FMath::FInterpTo(CurrentCameraRotation.Roll, AbsoluteTargetRoll, DeltaTime, CameraTiltInterpSpeed);
		FirstPersonCamera->SetRelativeRotation(FRotator(BaseCameraRotation.Pitch, BaseCameraRotation.Yaw, NewRoll));

		// --- HANDLE CAPSULE HALF HEIGHT INTERP FOR SLIDING ---
		if(GetCapsuleComponent())
		{
			float CurrentHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
			float NewHalfHeight = FMath::FInterpTo(CurrentHalfHeight, TargetHalfHeight, DeltaTime, SlideDropInterpSpeed);
			GetCapsuleComponent()->SetCapsuleHalfHeight(NewHalfHeight);
		}
	}
}

// Called to bind functionality to input
void AHordeShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if(EnhancedInputComponent)
	{
		//bind Move and Look using callbacks
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AHordeShooterCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AHordeShooterCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Canceled, this, &AHordeShooterCharacter::Move);

		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AHordeShooterCharacter::Look);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Completed, this, &AHordeShooterCharacter::Look);
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Canceled, this, &AHordeShooterCharacter::Look);

		//bind jump using built-in Jump functions
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AHordeShooterCharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AHordeShooterCharacter::StopJumping);
	
		//bind dash using callback
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &AHordeShooterCharacter::Dash);

		//bind slide using callbacks
		EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Started, this, &AHordeShooterCharacter::StartSlide);
		EnhancedInputComponent->BindAction(SlideAction, ETriggerEvent::Completed, this, &AHordeShooterCharacter::StopSlide);

		//bind fire using callback
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AHordeShooterCharacter::FireWeapon);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AHordeShooterCharacter::StopFiringWeapon);

		//bind switch weapon using callback
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Started, this, &AHordeShooterCharacter::SwitchWeapon);

		//bind aim using callbacks
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AHordeShooterCharacter::StartAiming);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AHordeShooterCharacter::StopAiming);
		
		//bind reload using callback
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AHordeShooterCharacter::ReloadWeapon);

		EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &AHordeShooterCharacter::TogglePause);
	
		EnhancedInputComponent->BindAction(GenerateArenaAction, ETriggerEvent::Started, this, &AHordeShooterCharacter::GenerateArena);
	}
}


void AHordeShooterCharacter::Move(const FInputActionValue& Value)
{
	MovementInputValue = Value;

	//disable movement input while sliding, as player must not be able to change direction while sliding.
	if(bIsSliding)
	{
		AddMovementInput(CachedInputDirection, 1.0f);
		return;
	}

	FVector2D MovementVector = Value.Get<FVector2D>();
	if(Controller != nullptr)
	{
		AddMovementInput(GetActorForwardVector(), MovementVector.Y); //forward/backward
		AddMovementInput(GetActorRightVector(), MovementVector.X); //right/left
	}
}

void AHordeShooterCharacter::Look(const FInputActionValue& Value)
{
	MouseInputValue = Value;

	FVector2D LookAxisVector = Value.Get<FVector2D>();
	if(Controller !=nullptr)
	{
		AddControllerYawInput(LookAxisVector.X); //yaw
		AddControllerPitchInput(-LookAxisVector.Y); //pitch
	}	
}

void AHordeShooterCharacter::Dash()
{
	if(AvailableDashes <= 0 || bIsDashing || bIsSliding || bIsAiming)
	{
		//DEBUG
		if (AvailableDashes <= 0) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("FAILED: Out of Dashes!"));
		else if (bIsSliding) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Orange, TEXT("FAILED: Cannot dash while sliding!"));
		else if(bIsAiming)  GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Orange, TEXT("FAILED: Cannot dash while aiming!"));

		return;
	}
	if(GetCharacterMovement()->IsFalling() && AirDashesUsed >= MaxDashes)
	{
		//DEBUG
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Red, TEXT("FAILED: Air Dash limit reached! Touch the ground."));		
		
		return;
	}

	//get input direction i.e. WASD:
	FVector InputDirection = GetLastMovementInputVector();

	//now calculate dash direction based on input direction:
	//-> if no WASD key pressed(no input dir), dash forward
	//-> else dash in input dir(first normalize input dir)
	CurrentDashDirection = (InputDirection.IsNearlyZero()) ? GetActorForwardVector() : InputDirection.GetSafeNormal();

	if(GetCharacterMovement()->IsMovingOnGround())
	{
		//get the normal of ground surface:
		FVector FloorNormal = GetCharacterMovement()->CurrentFloor.HitResult.ImpactNormal;

		//project dash direction onto the floor plane:
		CurrentDashDirection = FVector::VectorPlaneProject(CurrentDashDirection, FloorNormal).GetSafeNormal();

		//DEBUG
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::White, TEXT("Ground Dash Used!"));
	}
	else
	{
		CurrentDashDirection.Z = 0.f; //keep dash horizontal in air
		CurrentDashDirection.Normalize();
		AirDashesUsed++;

		//DEBUG
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Purple, FString::Printf(TEXT("Air Dash Used! (%d allowed)"), MaxDashes));
	}

	//trigger state:
	AvailableDashes--;
	bIsDashing = true;
	DashTimer = DashDuration;

	if (DashSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), DashSound, GetActorLocation());
	}

	//clear the timer in case player dashed again while it was recharging:
	GetWorldTimerManager().ClearTimer(DashTimerHandle);

	//handle cooldown:
	float CooldownTime = (AvailableDashes <= 0) ? DoubleDashCooldown : SingleDashCooldown;

	//DEBUG
	if (AvailableDashes <= 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Red, TEXT("Both dashes used! LONG PENALTY COOLDOWN STARTED."));
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Yellow, TEXT("One dash used! Short cooldown started."));
	}
	//DEBUG

	GetWorldTimerManager().SetTimer(
		DashTimerHandle,
		this,
		&AHordeShooterCharacter::RechargeDash,
		CooldownTime,
		false
	);	
}

void AHordeShooterCharacter::RechargeDash()
{
	if(AvailableDashes == 0)
	{
		AvailableDashes = MaxDashes;

		//DEBUG
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, TEXT("Penalty Over! ALL dashes restored."));
	}
	else if (AvailableDashes < MaxDashes)
	{
		//Just give one back
		AvailableDashes++;

		//DEBUG
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green, FString::Printf(TEXT("Dash Recharged! (%d/%d)"), AvailableDashes, MaxDashes));
	}
}

void AHordeShooterCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);

	if (AirDashesUsed > 0)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::Cyan, TEXT("Landed! Air Dashes Reset."));
		AirDashesUsed = 0;
	}
}

void AHordeShooterCharacter::StartSlide()
{
	if(bIsSliding || bIsDashing || bIsAiming)
	{
		//DEBUG
		if (bIsDashing) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Orange, TEXT("FAILED: Cannot slide while dashing!"));
		else if (bIsSliding) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Orange, TEXT("FAILED: Already sliding!"));
		else if (bIsAiming) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Orange, TEXT("FAILED: Cannot slide while aiming!"));

		return;
	}

	bIsSliding = true;
	TargetHalfHeight = CrouchedHalfHeight;

	if (SlideAudioComponent && SlideAudioComponent->Sound)
	{
		SlideAudioComponent->Play();
	}

	GetCharacterMovement()->MaxWalkSpeed = MaxSlideSpeed; 
	GetCharacterMovement()->GroundFriction = 0.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 1000.0f;

	CachedInputDirection = GetVelocity().GetSafeNormal2D();

	//if player presses slide with no inp direction
	if(CachedInputDirection.IsNearlyZero()) CachedInputDirection = GetActorForwardVector();

	if(GetCharacterMovement()->IsMovingOnGround())
	{
		GetCharacterMovement()->Velocity += FVector((CachedInputDirection * SlideSpeed).X, (CachedInputDirection * SlideSpeed).Y, 0.0f);
	}
	else
	{
		//if in air:
		GetCharacterMovement()->Velocity.Z -= 400.f;
	}
}

void AHordeShooterCharacter::StopSlide()
{
	bIsSliding = false;
	TargetHalfHeight = DefaultHalfHeight;

	if (SlideAudioComponent)
	{
		SlideAudioComponent->FadeOut(0.2f, 0.0f);
	}
	
	GetCharacterMovement()->MaxWalkSpeed = 1500.f;

	//reset friction and deceleration to default UE values.
	GetCharacterMovement()->GroundFriction = 8.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2048.0f;
}

void AHordeShooterCharacter::OnJumped_Implementation()
{
	Super::OnJumped_Implementation();

	if (JumpSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), JumpSound, GetActorLocation());
	}	
}

void AHordeShooterCharacter::PlayFootstepSound()
{
	if(FootstepSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), FootstepSound, GetActorLocation());
	}
}

void AHordeShooterCharacter::EquipWeapon(AHordeShooterWeapon* NewWeapon)
{
	if(!NewWeapon || NewWeapon == CurrentEquippedWeapon) return;

	AHordeShooterWeapon* PreviousWeapon = nullptr;

	//unequip/holster current weapon if we have one
	if(CurrentEquippedWeapon)
	{
		CurrentEquippedWeapon->SetActorHiddenInGame(true);
		CurrentEquippedWeapon->SetActorEnableCollision(false);
		CurrentEquippedWeapon->bIsEquipped = false;
		PreviousWeapon = CurrentEquippedWeapon;
	}

	CurrentEquippedWeapon = NewWeapon;
	CurrentEquippedWeapon->AttachToComponent(
		CharacterArms,
		FAttachmentTransformRules::KeepRelativeTransform,
		FName("ik_hand_gun")
	);

	CurrentEquippedWeapon->SetActorHiddenInGame(false);
	CurrentEquippedWeapon->SetActorEnableCollision(true);
	CurrentEquippedWeapon->bIsEquipped = true;

	if(AHordeShooterPlayerController* PC = Cast<AHordeShooterPlayerController>(GetController()))
	{
		if(PC->PlayerHUDWidget)
		{
			PC->PlayerHUDWidget->UpdateAmmo(CurrentEquippedWeapon->CurrentAmmo, CurrentEquippedWeapon->MagSize);
			CurrentEquippedWeapon->OnAmmoChanged.RemoveAll(PC->PlayerHUDWidget);

			//bind delegate to update HUD when ammo changes:
			CurrentEquippedWeapon->OnAmmoChanged.AddDynamic(PC->PlayerHUDWidget, &UHordeShooterHUDWidget::UpdateAmmo);
		}
	}

	float EquipTime = 0.15f; //fallback time
	if (CurrentEquippedWeapon->ArmsEquipMontage && CharacterArms)
	{
		if (UAnimInstance* AnimInst = CharacterArms->GetAnimInstance())
		{
			AnimInst->Montage_Play(CurrentEquippedWeapon->ArmsEquipMontage);
			EquipTime = CurrentEquippedWeapon->ArmsEquipMontage->GetPlayLength();
		}
	}

	//wait for the Equip animation to finish before unlocking the weapon
	GetWorldTimerManager().SetTimer(WeaponSwitchTimerHandle, this, &AHordeShooterCharacter::FinishEquipping, EquipTime, false);
}

void AHordeShooterCharacter::FinishEquipping()
{
	bIsSwitchingWeapons = false;
}

void AHordeShooterCharacter::FireWeapon()
{
	if(bIsSwitchingWeapons) return;

	if(CurrentEquippedWeapon)
	{
		CurrentEquippedWeapon->StartFire();
	}
}

void AHordeShooterCharacter::StopFiringWeapon()
{
	if(CurrentEquippedWeapon && !bIsSwitchingWeapons)
	{
		CurrentEquippedWeapon->StopFire();
	}
}

void AHordeShooterCharacter::SwitchWeapon(const FInputActionValue& Value)
{
	if(Inventory.Num() <= 1 || bIsSwitchingWeapons) return;
	
	const float ScrollValue = Value.Get<float>();
	if(FMath::IsNearlyZero(ScrollValue)) return;

	//force player out of aiming or firing:
	StopFiringWeapon();
	if(bIsAiming) StopAiming();

	const int32 Direction = (ScrollValue > 0.f) ? 1 : -1;
	int32 NextIndex = (CurrentWeaponIndex + Direction + Inventory.Num()) % Inventory.Num();
	PendingWeapon = Inventory[NextIndex];

	bIsSwitchingWeapons = true;
	if (CurrentEquippedWeapon)
	{
		CurrentEquippedWeapon->OnHolstered();
	}

	//play holster anim
	float HolsterTime = 0.15f; //fallback time just in case
	if (CurrentEquippedWeapon && CurrentEquippedWeapon->ArmsHolsterMontage && CharacterArms)
	{
		if (UAnimInstance* AnimInst = CharacterArms->GetAnimInstance())
		{
			AnimInst->Montage_Play(CurrentEquippedWeapon->ArmsHolsterMontage);
			HolsterTime = CurrentEquippedWeapon->ArmsHolsterMontage->GetPlayLength();
		}
	}

	//start timer, wait for anim to finish
	GetWorldTimerManager().SetTimer(WeaponSwitchTimerHandle, this, &AHordeShooterCharacter::PerformWeaponSwitch, HolsterTime, false);
}

void AHordeShooterCharacter::PerformWeaponSwitch()
{
	CurrentWeaponIndex = Inventory.Find(PendingWeapon);
	EquipWeapon(PendingWeapon);
}


void AHordeShooterCharacter::StartAiming()
{
	if(!CurrentEquippedWeapon || bIsSliding || bIsDashing || bIsSwitchingWeapons) return;
	
	if(CurrentEquippedWeapon->bCanAim)
	{
		bIsAiming = true;

		// Hide Crosshair
		if (AHordeShooterPlayerController* PC = Cast<AHordeShooterPlayerController>(GetController()))
		{
			if (PC->PlayerHUDWidget) PC->PlayerHUDWidget->ToggleCrosshair(false);
		}
	}
	else
	{
		//cant aim, hence try alt fire if available.
		CurrentEquippedWeapon->StartAltFire();
	}
}

void AHordeShooterCharacter::StopAiming()
{
	if(!CurrentEquippedWeapon || bIsSliding || bIsDashing) return;
	
	if(CurrentEquippedWeapon->bCanAim)
	{
		bIsAiming = false;

		// Show Crosshair
		if (AHordeShooterPlayerController* PC = Cast<AHordeShooterPlayerController>(GetController()))
		{
			if (PC->PlayerHUDWidget) PC->PlayerHUDWidget->ToggleCrosshair(true);
		}
	}
	else
	{
		//stop alt fire if it was active.
		CurrentEquippedWeapon->StopAltFire();
	}
	
}

void AHordeShooterCharacter::ReloadWeapon()
{
	if (bIsSwitchingWeapons) return;

	if(CurrentEquippedWeapon)
	{
		CurrentEquippedWeapon->Reload();
	}
}

bool AHordeShooterCharacter::IsCloseToWall()
{
	if(!FirstPersonCamera) return false;

	FVector Start = FirstPersonCamera->GetComponentLocation();
	FVector Direction = FirstPersonCamera->GetForwardVector();
	FVector End = Start + (Direction * WallCheckDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_Visibility,
		QueryParams
	);

	//DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Red : FColor::Green, false, -1.0f, 0, 2.0f);
	if(bHit && HitResult.GetActor() && HitResult.GetActor()->IsA(APawn::StaticClass())) return false; //ignore pawns, only check for walls
	else if(bHit && CurrentEquippedWeapon) CurrentEquippedWeapon->bIsLowered = true;
	else
	{
		if(CurrentEquippedWeapon) CurrentEquippedWeapon->bIsLowered = false;
	}

	return bHit;
}

bool AHordeShooterCharacter::ReactToHit(float DamageAmount, const FVector& HitImpulse, FName HitBoneName)
{
	if(CurrentHealth <= 0.f) return false; //already dead

	CurrentHealth -= DamageAmount;

	//update UI
	if(AHordeShooterPlayerController* PC = Cast<AHordeShooterPlayerController>(GetController()))
	{
		if(PC->PlayerHUDWidget) PC->PlayerHUDWidget->UpdateHealth(CurrentHealth, MaxHealth);
	}

	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FString::Printf(TEXT("PLAYER HIT! Health: %f"), CurrentHealth));
	
	if(CurrentHealth <= 0)
	{
		PlayerDie();
	}

	return false; //player takes normal damage.
}

void AHordeShooterCharacter::PlayerDie()
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("PLAYER IS DEAD. GAME OVER."));
	
	//disable player movement and actions
	GetCharacterMovement()->DisableMovement();
	bIsAiming = false;
	bIsSliding = false;
	bIsDashing = false;
	
	//hide/Disable weapon
	if(CurrentEquippedWeapon)
	{
		CurrentEquippedWeapon->StopFire();
		CurrentEquippedWeapon->bIsEquipped = false;
	}
	
	//tell the Controller to show the Game Over screen
	if(AHordeShooterPlayerController* PC = Cast<AHordeShooterPlayerController>(GetController()))
	{
		PC->ShowGameOverScreen();
	}
}

void AHordeShooterCharacter::TogglePause()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    bool bIsPaused = UGameplayStatics::IsGamePaused(GetWorld());

    if (bIsPaused)
    {
        // UNPAUSE
        UGameplayStatics::SetGamePaused(GetWorld(), false);

        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = false;
    }
    else
    {
        // PAUSE
        UGameplayStatics::SetGamePaused(GetWorld(), true);

        PC->SetInputMode(FInputModeUIOnly());
        PC->bShowMouseCursor = true;
    }
}

void AHordeShooterCharacter::GenerateArena()
{
	// Find the one and only Arena Manager in the world
	AActor* ArenaActor = UGameplayStatics::GetActorOfClass(GetWorld(), AArenaManager::StaticClass());
	
	if (AArenaManager* ArenaManager = Cast<AArenaManager>(ArenaActor))
	{
		ArenaManager->BeginNewLayoutGeneration();
		
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, TEXT("SYSTEM: Reconfiguring Arena Layout..."));
	}
}