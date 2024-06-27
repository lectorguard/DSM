// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DSMDataAsset.generated.h"

/**
 * Base data asset type used by DSM system
 * DataAssets used with DSM should be always of this type, e.g. default node referencing data assets 
 */
UCLASS(Blueprintable, MinimalAPI)
class UDSMDataAsset : public UDataAsset
{
	GENERATED_BODY()
public:
	// Make a copy of objects ptrs, to ensure consistency in deep copy situations
	// You can simply duplicate all ptr type objects in here
	// Please use the passed owner as new owner of the copied objects
	virtual void OnRequestDeepCopy(UObject* owner) {};
};
