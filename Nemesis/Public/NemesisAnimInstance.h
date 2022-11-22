// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "NemesisAnimInstance.generated.h"

/**
 * 
 */
UCLASS(transient, Blueprintable, hideCategories = AnimInstance, BlueprintType)
class NEMESIS_API UNemesisAnimInstance : public UAnimInstance
{
	GENERATED_BODY()


public:
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	//bool IsAttacking = false;
};
