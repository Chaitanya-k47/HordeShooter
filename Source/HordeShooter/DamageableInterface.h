// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "DamageableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UDamageableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class HORDESHOOTER_API IDamageableInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	//"= 0" means this is a pure virtual function.
	//any class that implements this interface must define this function.
	//hence this class is abstract
	virtual void ReactToHit(float DamageAmount, const FVector& HitImpulse, FName HitBoneName) = 0;

};
