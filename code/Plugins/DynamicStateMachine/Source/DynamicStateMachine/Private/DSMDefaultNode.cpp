// Fill out your copyright notice in the Description page of Project Settings.


#include "DSMDefaultNode.h"
#include "DSMLogInclude.h"
#include "DSMSaveGame.h"
#include "DSMPolicy.h"
#include "DSMCondition.h"
#include "DSMManager.h"


UDSMDefaultNode::UDSMDefaultNode()
{
	_nodePolicies = { UDSMDefaultPolicy::StaticClass() };
}

bool UDSMDefaultNode::ValidateConditionName(const FName& name) const
{
	// TODO Validate also condition object value + change UDataAsset* to name
	if (_ConditionDefinitions.Contains(name))
	{
		return true;
	}
	else
	{
		UE_LOG(LogDSM, Warning, TEXT("Condition Definition does not contain name with %s"), *name.ToString());
		return false;
	}
}

void UDSMDefaultNode::BeginPlay()
{
	Super::BeginPlay();
	if (!ValidateConditionGroups())
	{
		UE_LOG(LogDSM, Warning, TEXT("Conditions can not be validated for default node %s (outer : %s)"), *GetName(), *GetOuter()->GetName());
	}
	ADSMGameMode::RegisterNode(this);
}

void UDSMDefaultNode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	ADSMGameMode::UnregisterNode(this);
}

bool UDSMDefaultNode::RequestDSMSelfTransition()
{
	if (_requestSelfTranstion)
	{
		return _requestSelfTranstion(this);
	}
	return false;
}

void UDSMDefaultNode::GetData(FName key, TSubclassOf<UDSMDataAsset> castToType, UObject*& dataAsset, bool& success) const
{
	TObjectPtr<UObject> found = Cast<UObject>(GetData(key));
	if (IsValid(found))
	{
		dataAsset = found;
		success = true;
	}
	else
	{
		UE_LOG(LogDSM, Error, TEXT("GetData with key %s and type %s failed !"), *key.ToString(), *(castToType ? castToType->GetName() : FString("None")));
		dataAsset = nullptr;
		success = false;
	}
}

TWeakObjectPtr<UDSMDataAsset> UDSMDefaultNode::GetData(FName key) const
{
	const TWeakObjectPtr<ADSMGameMode> gameMode = GetDSMManager();
	if (!gameMode.IsValid())
	{
		UE_LOG(LogDSM, Error, TEXT("DSM game mode is invalid. Default Node %s (outer %s) can not access it."), *GetName(), *GetOuter()->GetName());
		return nullptr;
	}

	// XOR key can not be used in both or none of the data refs
	if (_writableDataReferences.Contains(key) == _readOnlyDataReferences.Contains(key))
	{
		UE_LOG(LogDSM, Error, TEXT("Key %s is used either in both writable and readonly refs or in none of them. Please update DSM node %s (outer %s)."), *key.ToString(), *GetName(), *GetOuter()->GetName());
		return nullptr;
	}

	if (_writableDataReferences.Contains(key))
	{
		if (_writableDataReferences[key])
		{
			return gameMode->GetDataAssetCached(_writableDataReferences[key]);
		}
		else
		{
			UE_LOG(LogDSM, Error, TEXT("Writable data asset contain nullptr, see %s outer %s"), *GetName(), *GetOuter()->GetName());
			return nullptr;
		}	
	}
	else
	{
		if (_readOnlyDataReferences[key])
		{
			return gameMode->_stateMachineData->GetDataCopy(_readOnlyDataReferences[key]);
		}
		else
		{
			UE_LOG(LogDSM, Error, TEXT("read only data asset contains nullptr, see %s outer %s"), *GetName(), *GetOuter()->GetName());
			return nullptr;
		}
	}
}

TWeakObjectPtr<UDSMSaveGame> UDSMDefaultNode::GetDSMSaveGame() const
{
	const TWeakObjectPtr<ADSMGameMode> gameMode = GetDSMManager();
	if (gameMode.IsValid())
	{
		return gameMode->_stateMachineData;
	}
	return nullptr;
}

TWeakObjectPtr<UDSMDataAsset> UDSMDefaultNode::ValidateDataAssetByName(const FName& name) const
{
	if (_readOnlyDataReferences.Contains(name))
	{
		return _readOnlyDataReferences[name];
	}
	if (_writableDataReferences.Contains(name))
	{
		return _writableDataReferences[name];
	}
	return nullptr;
}

bool UDSMDefaultNode::ValidateConditionGroups(bool reset /*= false*/)
{
	//Validate condition definitions first
	bool CondGrpValid = true;
	for (const TTuple<FName, TObjectPtr<UDSMConditionBase>> elem : _ConditionDefinitions)
	{
		if (!elem.Value || !elem.Value->IsValidLowLevel())
		{
			UE_LOG(LogDSM, Warning, TEXT("Condition definition %s has an invalid value"), *elem.Key.ToString());
			return false;
		}

		elem.Value->bIsBound = elem.Value->BindCondition(this);
		CondGrpValid = CondGrpValid && elem.Value->bIsBound;
		if (reset)
		{
			elem.Value->bIsBound = false;
		}
	}
	_expressionEvaluators = {};
	CondGrpValid = CondGrpValid && UDSMConditionUtils::ValidateConditionGroups(_ConditionGroups, [this](FName name) {return ValidateConditionName(name); }, _expressionEvaluators);
	return CondGrpValid;
}

bool UDSMDefaultNode::EvaluateConditionGroups() const
{
	return UDSMConditionUtils::EvaluateConditionGroups(this, _expressionEvaluators);
}

bool UDSMDefaultNode::EvaluateEnterConditions(bool IsSelfTransition, TTuple<FString, FDSMDebugConditions>& debugInfo) const
{
	FDSMDebugConditions debugElements; 

	bool bCanEnterResult = true;
	TMap<FName, bool> CanEnterResults = {};
	CanEnterState(IsSelfTransition, CanEnterResults);
	for (const TTuple<FName, bool>& elem : CanEnterResults)
	{
		debugElements.Conditions.Add(elem.Key, elem.Value);
		bCanEnterResult = bCanEnterResult && elem.Value;
	}

	bool bCanEnterEventResult = true;
	TMap<FName, bool> CanEnterEventResults = {};
	CanEnterStateEvent(IsSelfTransition, CanEnterEventResults);
	for (const TTuple<FName, bool>& elem : CanEnterEventResults)
	{
		debugElements.Conditions.Add(elem.Key, elem.Value);
		bCanEnterEventResult = bCanEnterEventResult && elem.Value;
	}

	bool bCanEnterConditionGroups = true;
	for (int32 i = 0; i < _expressionEvaluators.Num(); ++i)
	{
		const bool result = _expressionEvaluators[i].GetEvaluateFunction()(this);
		debugElements.Conditions.Add(_expressionEvaluators[i].name, result);
		bCanEnterConditionGroups = bCanEnterConditionGroups && result;
	}

	const FString nodeName = FString::Printf(TEXT("%s -> %s"), *(GetOuter() ? GetOuter()->GetName() : FString("None")), *GetName());
	debugInfo = { nodeName, debugElements };
	return bCanEnterResult && bCanEnterEventResult && bCanEnterConditionGroups;
}

#if WITH_EDITOR
void UDSMDefaultNode::PostInitProperties()
{
	Super::PostInitProperties();
	_BindConditions = ValidateConditionGroups();
}


void UDSMDefaultNode::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (bCanEnter)
	{
		bCanEnter = false;
		if (PropertyChangedEvent.Property != nullptr)
		{
			const FName PropertyName(PropertyChangedEvent.Property->GetFName());
			if (PropertyName == GET_MEMBER_NAME_CHECKED(UDSMDefaultNode, _ConditionDefinitions) ||
				PropertyName == GET_MEMBER_NAME_CHECKED(UDSMDefaultNode, _ConditionGroups) ||
				PropertyName == GET_MEMBER_NAME_CHECKED(UDSMDefaultNode, _BindConditions) && _BindConditions)
			{
				_BindConditions = ValidateConditionGroups();
			}
			if (PropertyName == GET_MEMBER_NAME_CHECKED(UDSMDefaultNode, _BindConditions) && !_BindConditions)
			{
				ValidateConditionGroups(true);
			}
		}
		bCanEnter = true;
	}
	
}
#endif

ADSMGameMode* UDSMDefaultNode::GetDSMManager() const
{
	if (!_ownerRef.IsValid())
	{
		UE_LOG(LogDSM, Warning, TEXT("Reference to owning dynamic state machine manager is invalid"));
	}
	return _ownerRef.Get();
}


