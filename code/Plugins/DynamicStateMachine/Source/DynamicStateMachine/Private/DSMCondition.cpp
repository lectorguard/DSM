// Fill out your copyright notice in the Description page of Project Settings.


#include "DSMCondition.h"
#include "DSMDefaultNode.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"


bool UDSMConditionBool::Evaluate(TWeakObjectPtr<const class UDSMDefaultNode> defaultNode) const
{
	if (defaultNode.IsValid())
	{
		const TWeakObjectPtr<UDSMDataAsset> foundDataAsset = Cast<UDSMDataAsset>(defaultNode->GetData(_DataAssetName));
		const bool* result = GetPropertyValueByName<bool, FBoolProperty>(foundDataAsset, _FieldName);
		if (result)
		{
			return *result;
		}
		UE_LOG(LogDSM, Error, TEXT("Bool condition with properties fieldName %s and dataAssetName %s is invalid."), *_FieldName.ToString(), *_DataAssetName.ToString());
	}
	return false;
}

bool UDSMConditionBool::BindCondition(TWeakObjectPtr<class UDSMDefaultNode> defaultNode) const
{
	if (defaultNode.IsValid())
	{
		const TWeakObjectPtr<UDSMDataAsset> foundDataAsset = defaultNode->ValidateDataAssetByName(_DataAssetName);
		if (foundDataAsset.IsValid())
		{
			if (const bool* value = GetPropertyValueByName<bool, FBoolProperty>(foundDataAsset.Get(), _FieldName))
			{
				return true;
			}
			UE_LOG(LogDSM, Warning, TEXT("Could not find property with variable name %s inside dataTable %s. Please make sure you removed all spaces in the name."), *_FieldName.ToString(), *_DataAssetName.ToString());
			return false;
		}
		UE_LOG(LogDSM, Warning, TEXT("DataAssetName %s is not contained in read/write data asset of the default node"), *_DataAssetName.ToString());
	}
	return false;
}

bool UDSMConditionComponentOverlap::Evaluate(TWeakObjectPtr<const class UDSMDefaultNode> defaultNode) const
{
	if (defaultNode.IsValid())
	{
		TWeakObjectPtr<UPrimitiveComponent> primitiveComponent = Cast<UPrimitiveComponent>(defaultNode->GetOwner()->GetDefaultSubobjectByName(_OwningComponentName));
		if (primitiveComponent.IsValid())
		{
			return primitiveComponent->IsOverlappingActor(Cast<AActor>(UGameplayStatics::GetPlayerCharacter(defaultNode->GetWorld(), 0)));
		}
	}
	return false;
}

bool UDSMConditionComponentOverlap::BindCondition(TWeakObjectPtr<class UDSMDefaultNode> defaultNode) const
{
	if (defaultNode.IsValid())
	{
		// Check inside editor
		TWeakObjectPtr<UBlueprintGeneratedClass> foundAsset = Cast<UBlueprintGeneratedClass>(defaultNode->GetOuter());
		if (foundAsset.IsValid())
		{
			TWeakObjectPtr<USimpleConstructionScript> constructionScript = foundAsset->SimpleConstructionScript;
			if (constructionScript.IsValid())
			{
				const TArray<USCS_Node*>& scsnodes = constructionScript->GetAllNodes();
				for (USCS_Node* Node : scsnodes)
				{
					if (UClass::FindCommonBase(Node->ComponentClass, UPrimitiveComponent::StaticClass()) == UPrimitiveComponent::StaticClass())
					{
						if (const UActorComponent* comp = Node->ComponentTemplate)
						{
							FString displayName = comp->GetName().Replace(TEXT("_GEN_VARIABLE"),TEXT(""));
							if (FName(*displayName) == _OwningComponentName)
							{
								return true;
							}
						}
					}
				}
			}	
		}
		// Check at runtime
		TWeakObjectPtr<UPrimitiveComponent> primitiveComponent = Cast<UPrimitiveComponent>(defaultNode->GetOwner()->GetDefaultSubobjectByName(_OwningComponentName));
		if (primitiveComponent.IsValid())
		{
			return true;
		}
	}
	UE_LOG(LogDSM, Warning, TEXT("Can not validate component %s"), *_OwningComponentName.ToString());
	return false;
}

bool UDSMConditionPointerValid::Evaluate(TWeakObjectPtr<const class UDSMDefaultNode> defaultNode) const
{
	if (defaultNode.IsValid())
	{
		TWeakObjectPtr<UDSMDataAsset> refDataAsset = Cast<UDSMDataAsset>(defaultNode->GetData(_DataAssetName));
		if (refDataAsset.IsValid())
		{
			if (FObjectProperty* foundProperty = CastField<FObjectProperty>(refDataAsset->GetClass()->FindPropertyByName(_FieldName)))
			{
				TWeakObjectPtr<UObject> value = Cast<UObject>(foundProperty->GetObjectPropertyValue_InContainer(refDataAsset.Get()));
				return value.IsValid();
			}
		}
	}
	return false;
}

bool UDSMConditionPointerValid::BindCondition(TWeakObjectPtr<class UDSMDefaultNode> defaultNode) const
{
	if (defaultNode.IsValid())
	{
		const TWeakObjectPtr<UDSMDataAsset> foundDataAsset = defaultNode->ValidateDataAssetByName(_DataAssetName);
		if (foundDataAsset.IsValid())
		{
			if(FObjectProperty* foundProperty = CastField<FObjectProperty>(foundDataAsset->GetClass()->FindPropertyByName(_FieldName)))
			{
				return true;
			}
			UE_LOG(LogDSM, Warning, TEXT("Could not find property with variable name %s inside dataTable %s. Please make sure you removed all spaces in the name."), *_FieldName.ToString(), *_DataAssetName.ToString());
			return false;
		}
		UE_LOG(LogDSM, Warning, TEXT("DataAssetName %s is not contained in read/write data asset of the default node"), *_DataAssetName.ToString());
	}
	return false;
}



bool UDSMConditionContainedInArray::Evaluate(TWeakObjectPtr<const class UDSMDefaultNode> defaultNode) const
{
	if (defaultNode.IsValid() &&
		!_DataAssetNameTarget.IsNone() &&
		!_FieldNameTarget.IsNone() &&
		!_DataAssetNameTargetShouldContain.IsNone() &&
		!_FieldNameTargetShouldContain.IsNone())
	{
		const TWeakObjectPtr<UDSMDataAsset> targetDA = Cast<UDSMDataAsset>(defaultNode->GetData(_DataAssetNameTarget));
		const TWeakObjectPtr<UDSMDataAsset> shouldContainDA = Cast<UDSMDataAsset>(defaultNode->GetData(_DataAssetNameTargetShouldContain));
		if (!targetDA.IsValid() || !shouldContainDA.IsValid())
		{
			UE_LOG(LogDSM, Warning, TEXT("Either data asset name %s or %s is invalid"), *_DataAssetNameTarget.ToString(), *_DataAssetNameTargetShouldContain.ToString());
			return false;
		}

		if (IsContaining<UClass*, FClassProperty>(targetDA, _FieldNameTarget, shouldContainDA, _FieldNameTargetShouldContain) ||
			IsContaining<FName, FNameProperty>(targetDA, _FieldNameTarget, shouldContainDA, _FieldNameTargetShouldContain))
		{
			return true;
		}
	}
	return false;
}

bool UDSMConditionContainedInArray::BindCondition(TWeakObjectPtr<class UDSMDefaultNode> defaultNode) const
{ 
	if (defaultNode.IsValid() &&
		!_DataAssetNameTarget.IsNone() &&
		!_FieldNameTarget.IsNone() &&
		!_DataAssetNameTargetShouldContain.IsNone() &&
		!_FieldNameTargetShouldContain.IsNone())
	{
		const TWeakObjectPtr<UDSMDataAsset> targetDA = Cast<UDSMDataAsset>(defaultNode->ValidateDataAssetByName(_DataAssetNameTarget));
		const TWeakObjectPtr<UDSMDataAsset> shouldContainDA = Cast<UDSMDataAsset>(defaultNode->ValidateDataAssetByName(_DataAssetNameTargetShouldContain));
		if (!targetDA.IsValid() || !shouldContainDA.IsValid())
		{
			UE_LOG(LogDSM, Warning, TEXT("Either data asset name %s or %s is invalid"), *_DataAssetNameTarget.ToString(), *_DataAssetNameTargetShouldContain.ToString());
			return false;
		}

		if (IsCastable<FClassProperty>(targetDA, _FieldNameTarget, shouldContainDA, _FieldNameTargetShouldContain) ||
			IsCastable<FNameProperty>(targetDA, _FieldNameTarget, shouldContainDA, _FieldNameTargetShouldContain))
		{
			return true;
		}
		else
		{
			UE_LOG(LogDSM, Warning, TEXT("Condition Contained Array type not suppoerted, supported types are UClass, FText"));
		}
	}
	return false;
}

void EvaluatePropertyChain(const TArray<FDSMConditionProperty>& inPropertyChain, TWeakObjectPtr<UObject> inData, TWeakObjectPtr<UObject>& outData, FProperty*& outProperty, bool IsValidation = false)
{
	if (!inData.IsValid())
	{
		return;
	}

	outData = inData;
	for (const FDSMConditionProperty& propInfo : inPropertyChain)
	{
		if (!outData.IsValid())
		{
			outData = nullptr;
			return;
		}

		if (FProperty* propertyType = CastField<FProperty>(outData->GetClass()->FindPropertyByName(propInfo._FieldName)))
		{
			// Define conversions here
			if (FObjectProperty* objectProperty = CastField<FObjectProperty>(propertyType))
			{
				TWeakObjectPtr<UObject> value = Cast<UObject>(objectProperty->GetObjectPropertyValue_InContainer(outData.Get()));
				if (value.IsValid())
				{
					outData = value;
					outProperty = objectProperty;
				}
				// Only during validation in editor
				else if (IsValidation && propInfo._OptionalValidationClass)
				{
					outData = propInfo._OptionalValidationClass->GetDefaultObject();
					outProperty = objectProperty;
				}
				else
				{
					outData = nullptr;
					outProperty = nullptr;
				}
			}
			else
			{
				outProperty = propertyType;
			}
		}
		else
		{
			UE_LOG(LogDSM, Warning, TEXT("Invalid Field with name %s"), *propInfo._FieldName.ToString());
			return;
		}
	}
}

bool UDSMConditionCompare::Evaluate(TWeakObjectPtr<const class UDSMDefaultNode> defaultNode) const
{
	if (!defaultNode.IsValid())
	{
		return false;
	}

	const TWeakObjectPtr<UDSMDataAsset> leftDataAsset = Cast<UDSMDataAsset>(defaultNode->GetData(_DataAssetLeft));
	const TWeakObjectPtr<UDSMDataAsset> rightDataAsset = Cast<UDSMDataAsset>(defaultNode->GetData(_DataAssetRight));
	TWeakObjectPtr<UObject> leftData = nullptr;
	FProperty* leftProperty = nullptr;
	TWeakObjectPtr<UObject> rightData = nullptr;
	FProperty* rightProperty = nullptr;
	EvaluatePropertyChain(_PropertyChainLeft, leftDataAsset, leftData, leftProperty);
	EvaluatePropertyChain(_PropertyChainRight, rightDataAsset, rightData, rightProperty);

	if (!leftProperty || !rightProperty)
	{
		return false;
	}

	if (rightProperty->SameType(leftProperty))
	{
		return rightProperty->Identical_InContainer(leftData.Get(), rightData.Get());
	}
	else
	{
		UE_LOG(LogDSM, Warning, TEXT("Types you want to compare must be identical"));
		return false;
	}
}

bool UDSMConditionCompare::BindCondition(TWeakObjectPtr<class UDSMDefaultNode> defaultNode) const
{
	if (_DataAssetLeft.IsNone() || _PropertyChainLeft.IsEmpty() || _DataAssetRight.IsNone() || _PropertyChainRight.IsEmpty() || !defaultNode.IsValid())
	{
		return false;
	}

	const TWeakObjectPtr<UDSMDataAsset> leftDataAsset = Cast<UDSMDataAsset>(defaultNode->ValidateDataAssetByName(_DataAssetLeft));
	const TWeakObjectPtr<UDSMDataAsset> rightDataAsset = Cast<UDSMDataAsset>(defaultNode->ValidateDataAssetByName(_DataAssetRight));
	if (!leftDataAsset.IsValid() || !rightDataAsset.IsValid())
	{
		UE_LOG(LogDSM, Warning, TEXT("At least one of the data assets to compare is invalid"));
		return false;
	}

	TWeakObjectPtr<UObject> leftData = nullptr;
	FProperty* leftProperty = nullptr;
	TWeakObjectPtr<UObject> rightData = nullptr;
	FProperty* rightProperty = nullptr;
	EvaluatePropertyChain(_PropertyChainLeft, leftDataAsset, leftData, leftProperty, true);
	EvaluatePropertyChain(_PropertyChainRight, rightDataAsset, rightData, rightProperty, true);

	if (!leftProperty || !rightProperty)
	{
		return false;
	}


	if (rightProperty->SameType(leftProperty))
	{
		return true;
	}
	else
	{
		UE_LOG(LogDSM, Warning, TEXT("Types you want to compare must be identical"));
		return false;
	}
}

bool UDSMConditionNumberTypeCompare::Evaluate(TWeakObjectPtr<const class UDSMDefaultNode> defaultNode) const
{
	if (!defaultNode.IsValid())
	{
		return false;
	}

	const TWeakObjectPtr<UDSMDataAsset> leftDataAsset = Cast<UDSMDataAsset>(defaultNode->GetData(_DataAssetLeft));
	const TWeakObjectPtr<UDSMDataAsset> rightDataAsset = Cast<UDSMDataAsset>(defaultNode->GetData(_DataAssetRight));
	TWeakObjectPtr<UObject> leftData = nullptr;
	FProperty* leftProperty = nullptr;
	TWeakObjectPtr<UObject> rightData = nullptr;
	FProperty* rightProperty = nullptr;
	EvaluatePropertyChain(_PropertyChainLeft, leftDataAsset, leftData, leftProperty);
	EvaluatePropertyChain(_PropertyChainRight, rightDataAsset, rightData, rightProperty);

	if (!leftProperty || !rightProperty)
	{
		return false;
	}

	if (rightProperty->SameType(leftProperty))
	{
		// float
		const FFloatProperty* leftFloatProp = CastField<FFloatProperty>(leftProperty);
		const FFloatProperty* rightFloatProp = CastField<FFloatProperty>(rightProperty);
		if (leftFloatProp && rightFloatProp)
		{
			const float* leftFloat = leftFloatProp->ContainerPtrToValuePtr<float>(leftData.Get());
			const float* rightFloat = rightFloatProp->ContainerPtrToValuePtr<float>(rightData.Get());
			if (leftFloat && rightFloat)
			{
				return CompareConditionNumber(*leftFloat, *rightFloat);
			}
			UE_LOG(LogDSM, Warning, TEXT("Invalid float property"));
			return false;
		}
		

		// int32
		const FIntProperty* leftIntProp = CastField<FIntProperty>(leftProperty);
		const FIntProperty* rightIntProp = CastField<FIntProperty>(rightProperty);
		if (leftIntProp && rightIntProp)
		{
			const int32* leftInt = leftIntProp->ContainerPtrToValuePtr<int32>(leftData.Get());
			const int32* rightInt = rightIntProp->ContainerPtrToValuePtr<int32>(rightData.Get());
			if (leftInt && rightInt)
			{
				return CompareConditionNumber(*leftInt, *rightInt);
			}
			UE_LOG(LogDSM, Warning, TEXT("Invalid int property"));
			return false;
		}

		UE_LOG(LogDSM, Warning, TEXT("Currently only types float and int32 are supported"));
		return false;
	}
	else
	{
		UE_LOG(LogDSM, Warning, TEXT("Types you want to compare must be identical"));
		return false;
	}
}

bool UDSMConditionNumberTypeCompare::BindCondition(TWeakObjectPtr<class UDSMDefaultNode> defaultNode) const
{
	if (_DataAssetLeft.IsNone() || _PropertyChainLeft.IsEmpty() || _DataAssetRight.IsNone() || _PropertyChainRight.IsEmpty() || !defaultNode.IsValid())
	{
		return false;
	}

	if (_CompareType == ENumberComparison::NONE)
	{
		UE_LOG(LogDSM, Warning, TEXT("Compare type can not be none"));
		return false;
	}

	const TWeakObjectPtr<UDSMDataAsset> leftDataAsset = Cast<UDSMDataAsset>(defaultNode->ValidateDataAssetByName(_DataAssetLeft));
	const TWeakObjectPtr<UDSMDataAsset> rightDataAsset = Cast<UDSMDataAsset>(defaultNode->ValidateDataAssetByName(_DataAssetRight));
	if (!leftDataAsset.IsValid() || !rightDataAsset.IsValid())
	{
		UE_LOG(LogDSM, Warning, TEXT("At least one of the data assets to compare is invalid"));
		return false;
	}

	TWeakObjectPtr<UObject> leftData = nullptr;
	FProperty* leftProperty = nullptr;
	TWeakObjectPtr<UObject> rightData = nullptr;
	FProperty* rightProperty = nullptr;
	EvaluatePropertyChain(_PropertyChainLeft, leftDataAsset, leftData, leftProperty, true);
	EvaluatePropertyChain(_PropertyChainRight, rightDataAsset, rightData, rightProperty, true);

	if (!leftProperty || !rightProperty)
	{
		return false;
	}


	if (rightProperty->SameType(leftProperty))
	{
		return true;
	}
	else
	{
		UE_LOG(LogDSM, Warning, TEXT("Types you want to compare must be identical"));
		return false;
	}
}
