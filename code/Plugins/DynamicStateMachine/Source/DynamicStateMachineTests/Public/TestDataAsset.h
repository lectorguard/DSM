// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DSMDataAsset.h"
#include "TestDataAsset.generated.h"

/**
 * 
 */
UCLASS()
class DYNAMICSTATEMACHINETESTS_API UTestDataAsset : public UDSMDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere, Category="True")
	bool bTrue = true;

	UPROPERTY(EditAnywhere, Category = "False")
	bool bFalse = false;
};

