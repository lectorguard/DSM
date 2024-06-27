// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DSMLogInclude.h"
#include "DSMDataAsset.h"
#include "DSMCondition.generated.h"

/**
 * Grabs a property based on type from a data asset
 */
template<typename ValueType, typename PropertyType>
ValueType* GetPropertyValueByName(const TWeakObjectPtr<UObject>& dataAsset, const FName& fieldName)
{
	if (dataAsset.IsValid() && !fieldName.IsNone())
	{
		if (PropertyType* propertyType = CastField<PropertyType>(dataAsset->GetClass()->FindPropertyByName(fieldName)))
		{
			if (ValueType* value = propertyType->ContainerPtrToValuePtr<ValueType>(dataAsset.Get()))
			{
				return value;
			}
		}
	}
	return nullptr;
}



/**
 * Base condition class, inherit from this class to create a custom condition.
 * Conditions are used to define DSM node enter conditions
 * Create custom conditions based on your game
 */
UCLASS(Blueprintable)
class DYNAMICSTATEMACHINE_API UDSMConditionBase : public UObject
{
	GENERATED_BODY()
	
public:
	// Called at runtime to evaluate the current value of the defined condition, must be overriden
	virtual bool Evaluate(TWeakObjectPtr<const class UDSMDefaultNode> defaultNode) const { return false; };

	// Called inside the editor to validate the correctness of the input
	virtual bool BindCondition(TWeakObjectPtr<class UDSMDefaultNode> defaultNode) const { return false; };

	// If true, condition could be validated correctly and condition was successfully bound
	// Flag is set based on the return value of Evaluate function, basically cached result
	UPROPERTY()
	bool bIsBound = false;
};

//////////////// CUSTOM CONDITIONS ////////////////////////////////////////////////////

/**
 * Condition which always evaluates to true. Used for testing
 */
UCLASS(Blueprintable, DefaultToInstanced, editinlinenew)
class DYNAMICSTATEMACHINE_API UDSMConditionTrue : public UDSMConditionBase
{
	GENERATED_BODY()

public:
	bool Evaluate(TWeakObjectPtr<const class UDSMDefaultNode> defaultNode) const override { return true; };
	bool BindCondition(TWeakObjectPtr<class UDSMDefaultNode> defaultNode) const override { return true; };
};

/**
 * Condition which always evaluates to false. Used for testing
 */
UCLASS(Blueprintable, DefaultToInstanced, editinlinenew)
class DYNAMICSTATEMACHINE_API UDSMConditionFalse : public UDSMConditionBase
{
	GENERATED_BODY()

public:
	bool Evaluate(TWeakObjectPtr<const class UDSMDefaultNode> defaultNode) const override { return false; };
	bool BindCondition(TWeakObjectPtr<class UDSMDefaultNode> defaultNode) const override { return true; };
};


/**
 * Condition binding a boolean value from a data asset
 */
UCLASS(Blueprintable, DefaultToInstanced, editinlinenew)
class DYNAMICSTATEMACHINE_API UDSMConditionBool : public UDSMConditionBase
{
	GENERATED_BODY()

public:
	// Name of data asset containing the bool property
	// DataAsset must be defined inside _writableDataReferences or _readOnlyDataReferences of the owning DSM default node
	UPROPERTY(EditAnywhere, Category = "Condition", meta = (EditCondition = "!bIsBound"))
	FName _DataAssetName = NAME_None;

	// Bool field name in data asset
	UPROPERTY(EditDefaultsOnly, Category = "Condition", meta = (EditCondition = "!bIsBound"))
	FName _FieldName = NAME_None;

	bool Evaluate(TWeakObjectPtr<const class UDSMDefaultNode> defaultNode) const override;
	bool BindCondition(TWeakObjectPtr<class UDSMDefaultNode> defaultNode) const override;
};

/**
 * Checks if player overlaps a component of the owning actor (owner of the default node)
 */
UCLASS(Blueprintable, DefaultToInstanced, editinlinenew)
class DYNAMICSTATEMACHINE_API UDSMConditionComponentOverlap : public UDSMConditionBase
{
	GENERATED_BODY()

public:
	// Overlappable component name of the owning actor 
	UPROPERTY(EditAnywhere, Category = "Condition", meta = (EditCondition = "!bIsBound"))
	FName _OwningComponentName = NAME_None;

	bool Evaluate(TWeakObjectPtr<const class UDSMDefaultNode> defaultNode) const override;
	bool BindCondition(TWeakObjectPtr<class UDSMDefaultNode> defaultNode) const override;
};

/**
 * Checks if object pointer in referenced data asset is valid
 */
UCLASS(Blueprintable, DefaultToInstanced, editinlinenew)
class DYNAMICSTATEMACHINE_API UDSMConditionPointerValid : public UDSMConditionBase
{
	GENERATED_BODY()

public:

	// Name of data asset containing the pointer property
	// DataAsset must be defined inside _writableDataReferences or _readOnlyDataReferences of the owning DSM default node
	UPROPERTY(EditAnywhere, Category = "Condition", meta = (EditCondition = "!bIsBound"))
	FName _DataAssetName = NAME_None;

	// Object pointer field name
	UPROPERTY(EditDefaultsOnly, Category = "Condition", meta = (EditCondition = "!bIsBound"))
	FName _FieldName = NAME_None;

	bool Evaluate(TWeakObjectPtr<const class UDSMDefaultNode> defaultNode) const override;
	bool BindCondition(TWeakObjectPtr<class UDSMDefaultNode> defaultNode) const override;
};

/**
 * Checks if a property is contained inside an array. Currently supported types are FName and UClass
 */
UCLASS(Blueprintable, DefaultToInstanced, editinlinenew)
class DYNAMICSTATEMACHINE_API UDSMConditionContainedInArray : public UDSMConditionBase
{
	GENERATED_BODY()


	template<typename T>
	bool IsCastable(TWeakObjectPtr<UDSMDataAsset> targetDA, FName targetName, TWeakObjectPtr<UDSMDataAsset> shouldContainDA, FName shouldContainName) const
	{
		if (targetDA.IsValid() && !targetName.IsNone() && shouldContainDA.IsValid() && !shouldContainName.IsNone())
		{
			T* targetArray = nullptr;
			T* shouldContainArray = nullptr;
			T* shouldContainElement = nullptr;
			if (FArrayProperty* propertyType = CastField<FArrayProperty>(targetDA->GetClass()->FindPropertyByName(targetName)))
			{
				targetArray = CastField<T>(propertyType->Inner);
			}

			if (FArrayProperty* propertyType = CastField<FArrayProperty>(shouldContainDA->GetClass()->FindPropertyByName(shouldContainName)))
			{
				shouldContainArray = CastField<T>(propertyType->Inner);
			}

			if (T* propertyType = CastField<T>(shouldContainDA->GetClass()->FindPropertyByName(shouldContainName)))
			{
				shouldContainElement = propertyType;
			}
			return !!targetArray && (!!shouldContainElement || !!shouldContainArray);
		}
		return false;
	}

	template<typename T, typename P>
	bool IsContaining(TWeakObjectPtr<UDSMDataAsset> targetDA, FName targetName, TWeakObjectPtr<UDSMDataAsset> shouldContainDA, FName shouldContainName) const
	{
		if (targetDA.IsValid() && !targetName.IsNone() && shouldContainDA.IsValid() && !shouldContainName.IsNone())
		{
			const TArray<T>* targetArray = GetPropertyValueByName<TArray<T>, FArrayProperty>(targetDA, targetName);
			const TArray<T>* shouldContainArray = GetPropertyValueByName<TArray<T>, FArrayProperty>(shouldContainDA, shouldContainName);
			const T* shouldContainElement = GetPropertyValueByName<T, P>(shouldContainDA, shouldContainName);
			if (targetArray && shouldContainArray)
			{
				bool success = true;
				for (auto element : *shouldContainArray)
				{
					if (!targetArray->Contains(element))
					{
						success = false;
					}
				}
				return success;
			}
			if (targetArray && shouldContainElement)
			{
				return targetArray->Contains(*shouldContainElement);
			}
		}
		return false;
	}

public:
	// Name of data asset containing the target array
	// DataAsset must be defined inside _writableDataReferences or _readOnlyDataReferences of the owning DSM default node
	UPROPERTY(EditAnywhere, Category = "Target", meta = (EditCondition = "!bIsBound"))
	FName _DataAssetNameTarget= NAME_None;

	// Field name of array in data asset
	UPROPERTY(EditDefaultsOnly, Category = "Target", meta = (EditCondition = "!bIsBound"))
	FName _FieldNameTarget = NAME_None;

	// Name of data asset in which is the property you want to test against
	// DataAsset must be defined inside _writableDataReferences or _readOnlyDataReferences of the owning DSM default node
	UPROPERTY(EditAnywhere, Category = "Should Contain", meta = (EditCondition = "!bIsBound"))
	FName _DataAssetNameTargetShouldContain = NAME_None;

	// Field name of the property you want to test against
	UPROPERTY(EditDefaultsOnly, Category = "Should Contain", meta = (EditCondition = "!bIsBound"))
	FName _FieldNameTargetShouldContain = NAME_None;

	bool Evaluate(TWeakObjectPtr<const class UDSMDefaultNode> defaultNode) const override;
	bool BindCondition(TWeakObjectPtr<class UDSMDefaultNode> defaultNode) const override;
};

UENUM(BlueprintType)
enum class EDSMPropertyType : uint8 {
	NONE,
	NAME,
	Object,
};


USTRUCT(Blueprintable)
struct DYNAMICSTATEMACHINE_API FDSMConditionProperty 
{
	GENERATED_BODY()

	// Target field name
	UPROPERTY(EditAnywhere, Category = "Condition")
	FName _FieldName = NAME_None;

	// Only used for validation, class of the field
	UPROPERTY(EditAnywhere, Category = "Validation")
	TSubclassOf<UObject> _OptionalValidationClass = nullptr;
};

/**
 * Compare 2 property field. Supports nested properties.
 */
UCLASS(Blueprintable, DefaultToInstanced, editinlinenew)
class DYNAMICSTATEMACHINE_API UDSMConditionCompare : public UDSMConditionBase
{
	GENERATED_BODY()

public:
	// Name of data asset which contains the property on the left side for comaprison
	// DataAsset must be defined inside _writableDataReferences or _readOnlyDataReferences of the owning DSM default node
	UPROPERTY(EditAnywhere, Category = "Left",  meta = (EditCondition = "!bIsBound"))
	FName _DataAssetLeft = NAME_None;

	// Property name chain on the left side, nested properties can be give here as a list
	UPROPERTY(EditDefaultsOnly, Category = "Left", meta = (EditCondition = "!bIsBound"))
	TArray<FDSMConditionProperty> _PropertyChainLeft = {};

	// Name of data asset which contains the property on the right side for comaprison
	// DataAsset must be defined inside _writableDataReferences or _readOnlyDataReferences of the owning DSM default node
	UPROPERTY(EditAnywhere, Category = "Right", meta = (EditCondition = "!bIsBound"))
	FName _DataAssetRight = NAME_None;

	// Property name chain on the right side, nested properties can be give here as a list
	UPROPERTY(EditDefaultsOnly, Category = "Right", meta = (EditCondition = "!bIsBound"))
	TArray<FDSMConditionProperty> _PropertyChainRight = {};

	bool Evaluate(TWeakObjectPtr<const class UDSMDefaultNode> defaultNode) const override;
	bool BindCondition(TWeakObjectPtr<class UDSMDefaultNode> defaultNode) const override;
};

UENUM(BlueprintType)
enum class ENumberComparison : uint8 {
	NONE,
	EQUAL,
	NONEQUAL,
	GREATER,
	SMALLER,
	GREATEREQUAL,
	SMALLEREQUAL
};

/**
 * Numeric comparison of 2 numeric types, Supported types are int32 and float
 */
UCLASS(Blueprintable, DefaultToInstanced, editinlinenew)
class DYNAMICSTATEMACHINE_API UDSMConditionNumberTypeCompare : public UDSMConditionBase
{
	GENERATED_BODY()

	template<typename T>
	bool CompareConditionNumber(T&& left,T&& right) const
	{
		switch (_CompareType)
		{
		case ENumberComparison::EQUAL:
			return left == right;
		case ENumberComparison::NONEQUAL:
			return left != right;
		case ENumberComparison::GREATER:
			return left > right;
		case ENumberComparison::SMALLER:
			return left < right;
		case ENumberComparison::GREATEREQUAL:
			return left >= right;
		case ENumberComparison::SMALLEREQUAL:
			return left <= right;
		default:
			UE_LOG(LogDSM, Warning, TEXT("Compare type invalid."));
			return false;
		}
	}

public:
	// Name of data asset which contains the property on the left side for comaprison
	// DataAsset must be defined inside _writableDataReferences or _readOnlyDataReferences of the owning DSM default node
	UPROPERTY(EditAnywhere, Category = "Left", meta = (EditCondition = "!bIsBound"))
	FName _DataAssetLeft = NAME_None;

	// Property name chain on the left side, nested properties can be give here as a list
	// Last property in the list must be a numeric type
	UPROPERTY(EditDefaultsOnly, Category = "Left", meta = (EditCondition = "!bIsBound"))
	TArray<FDSMConditionProperty> _PropertyChainLeft = {};

	// Comparison type
	UPROPERTY(EditAnywhere, Category = "ComparisonType", meta = (EditCondition = "!bIsBound"))
	ENumberComparison _CompareType = ENumberComparison::NONE;

	// Name of data asset which contains the property on the right side for comaprison
	// DataAsset must be defined inside _writableDataReferences or _readOnlyDataReferences of the owning DSM default node
	UPROPERTY(EditAnywhere, Category = "Right", meta = (EditCondition = "!bIsBound"))
	FName _DataAssetRight = NAME_None;

	// Property name chain on the left side, nested properties can be give here as a list
	// Last property in the list must be a numeric type
	UPROPERTY(EditDefaultsOnly, Category = "Right", meta = (EditCondition = "!bIsBound"))
	TArray<FDSMConditionProperty> _PropertyChainRight = {};

	bool Evaluate(TWeakObjectPtr<const class UDSMDefaultNode> defaultNode) const override;
	bool BindCondition(TWeakObjectPtr<class UDSMDefaultNode> defaultNode) const override;
};