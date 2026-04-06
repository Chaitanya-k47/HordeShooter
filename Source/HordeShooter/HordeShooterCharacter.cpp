// Fill out your copyright notice in the Description page of Project Settings.


#include "HordeShooterCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/SceneComponent.h"

#include "HordeShooterWeapon.h"

// Sets default values
AHordeShooterCharacter::AHordeShooterCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	GetMesh()->SetTickGroup(ETickingGroup::TG_PostUpdateWork);

	//Setup first person camera and mesh:
	Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("Pivot"));
	Pivot->SetupAttachment(GetRootComponent());

	CharacterArms = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterArms"));
	CharacterArms->SetupAttachment(Pivot);

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(CharacterArms, FName("head")); //attach camera to Head bone.
	FirstPersonCamera->bUsePawnControlRotation = false;


	//Movement config:
	GetCharacterMovement()->MaxWalkSpeed = 1000.f; // Base run speed (up from 600)
	GetCharacterMovement()->MaxAcceleration = 4000.f; // Snappy start
	GetCharacterMovement()->BrakingDecelerationWalking = 4000.f; // Snappy stop (no ice skating)
	GetCharacterMovement()->GroundFriction = 8.0f; 
	
	GetCharacterMovement()->GravityScale = 2.f;
	GetCharacterMovement()->JumpZVelocity = 1000.f;
	GetCharacterMovement()->AirControl = 0.8f;
	GetCharacterMovement()->FallingLateralFriction = 3.0f;
}

// Called when the game starts or when spawned
void AHordeShooterCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	//initialize target FOV to base FOV
	TargetFOV = IdleFOV;

	if(FirstPersonCamera)
	{
		BaseCameraLocation = FirstPersonCamera->GetRelativeLocation();
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
		if(Index == CurrentWeaponIndex)
		{
			//equip weapon.
			EquipWeapon(SpawnedWeapon);
		}
	}
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

		// STATE 1: DASHING
		if (bIsDashing)
		{
			TargetFOV = FMath::Lerp(IdleFOV, DashFOV, ForwardFactor);
			CurrentInterpSpeed = DashFOVInterpSpeed;

			//FVector LagOffset = FVector(0.f, -RightAlignment * MaxDashCameraLag, 0.f);
			//FVector LagOffset = FVector(-RightAlignment * MaxDashCameraLag, 0.f, 0.f);
			
			//this one works the best:
			FVector LagOffset = FVector(0.f, 0.f, -RightAlignment * MaxDashCameraLag);
			TargetCameraLocation = BaseCameraLocation + LagOffset;
		}
		// STATE 2: RUNNING
		else if (CurrentSpeed > 10.f)
		{
			TargetFOV = RunFOV; // Standard run FOV
			CurrentInterpSpeed = RunFOVInterpSpeed;
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

		// --- APPLY CAMERA TRANSLATION LAG ---
		FVector CurrentCameraLocation = FirstPersonCamera->GetRelativeLocation();
		FVector NewCameraLocation = FMath::VInterpTo(CurrentCameraLocation, TargetCameraLocation, DeltaTime, CameraLagInterpSpeed);
		FirstPersonCamera->SetRelativeLocation(NewCameraLocation);

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
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AHordeShooterCharacter::Look);

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

		//bind switch weapon using callback
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Started, this, &AHordeShooterCharacter::SwitchWeapon);

		//bind aim using callbacks
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &AHordeShooterCharacter::StartAiming);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &AHordeShooterCharacter::StopAiming);
		
		EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &AHordeShooterCharacter::TogglePause);
	}
}


void AHordeShooterCharacter::Move(const FInputActionValue& Value)
{
	FVector2D MovementVector = Value.Get<FVector2D>();
	if(Controller != nullptr)
	{
		AddMovementInput(GetActorForwardVector(), MovementVector.Y); //forward/backward
		AddMovementInput(GetActorRightVector(), MovementVector.X); //right/left
	}
}

void AHordeShooterCharacter::Look(const FInputActionValue& Value)
{
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	if(Controller !=nullptr)
	{
		AddControllerYawInput(LookAxisVector.X); //yaw
		AddControllerPitchInput(-LookAxisVector.Y); //pitch
	}	
}

void AHordeShooterCharacter::Dash()
{
	if(AvailableDashes <= 0 || bIsDashing || bIsSliding)
	{
		//DEBUG
		if (AvailableDashes <= 0) GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Red, TEXT("FAILED: Out of Dashes!"));
		else if (bIsSliding) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Orange, TEXT("FAILED: Cannot dash while sliding!"));

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
	if(bIsSliding || bIsDashing)
	{
		//DEBUG
		if (bIsDashing) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Orange, TEXT("FAILED: Cannot slide while dashing!"));
		else if (bIsSliding) GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Orange, TEXT("FAILED: Already sliding!"));

		return;
	}

	bIsSliding = true;
	TargetHalfHeight = CrouchedHalfHeight;

	if(GetCharacterMovement()->IsMovingOnGround())
	{
		FVector SlideDirection = GetVelocity().GetSafeNormal();
		if (SlideDirection.IsNearlyZero()) SlideDirection = GetActorForwardVector();

		GetCharacterMovement()->Velocity += FVector((SlideDirection * SlideSpeed).X, (SlideDirection * SlideSpeed).Y, -200.f);

		GetCharacterMovement()->MaxWalkSpeed = 300.f; 
		GetCharacterMovement()->GroundFriction = 0.0f;
		GetCharacterMovement()->BrakingDecelerationWalking = 1000.0f;
	}
}

void AHordeShooterCharacter::StopSlide()
{
	bIsSliding = false;
	TargetHalfHeight = DefaultHalfHeight;

	
	GetCharacterMovement()->MaxWalkSpeed = 1000.f;

	//reset friction and deceleration to default UE values.
	GetCharacterMovement()->GroundFriction = 8.0f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2048.0f;
}

void AHordeShooterCharacter::EquipWeapon(AHordeShooterWeapon* NewWeapon)
{
	if(!NewWeapon || NewWeapon == CurrentEquippedWeapon) return;

	AHordeShooterWeapon* PreviousWeapon = nullptr;

	//unequip/holster current weapon if we have one
	if(CurrentEquippedWeapon)
	{
		CurrentEquippedWeapon->Mesh->SetVisibility(false);
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

	CurrentEquippedWeapon->Mesh->SetVisibility(true);
	CurrentEquippedWeapon->SetActorEnableCollision(true);
	CurrentEquippedWeapon->bIsEquipped = true;
}

void AHordeShooterCharacter::FireWeapon()
{
	
}

void AHordeShooterCharacter::SwitchWeapon(const FInputActionValue& Value)
{
	if(Inventory.Num() <= 1) return;
	
	const float ScrollValue = Value.Get<float>();
	if(FMath::IsNearlyZero(ScrollValue)) return;

	const int32 Direction = (ScrollValue > 0.f) ? 1 : -1;
	CurrentWeaponIndex = (CurrentWeaponIndex + Direction + Inventory.Num()) % Inventory.Num();

	EquipWeapon(Inventory[CurrentWeaponIndex]);
}

void AHordeShooterCharacter::StartAiming()
{
	if(!CurrentEquippedWeapon) return;
	bIsAiming = true;
}

void AHordeShooterCharacter::StopAiming()
{
	if(!CurrentEquippedWeapon) return;
	bIsAiming = false;
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

