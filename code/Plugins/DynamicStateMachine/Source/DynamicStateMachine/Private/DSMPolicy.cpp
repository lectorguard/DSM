// Fill out your copyright notice in the Description page of Project Settings.


#include "DSMPolicy.h"
#include "DSMLogInclude.h"
#include "DSMManager.h"
#include "GameFramework/GameState.h"
#include "DSMDefaultNode.h"
#include "Kismet/GameplayStatics.h"

UObject* UDSMPolicy::FindFirstElementByClass(const TArray<UObject*>& inputObjects, TSubclassOf<UObject> assetType) const
{

	return *inputObjects.FindByPredicate([inputObjects, assetType](UObject* obj)
		{
			if (obj)
			{
				return obj->IsA(assetType);
			}
			else
			{
				return false;
			}
		});

}

TArray<UObject*> UDSMPolicy::FilterByClass(const TArray<UObject*>& inputObjects, TSubclassOf<UObject> filterType) const
{
	return inputObjects.FilterByPredicate([filterType](UObject* obj)
		{
			if (obj)
			{
				return obj->IsA(filterType);
			}
			else
			{
				return false;
			}
		});
}

void UDSMPolicy::ActivatePolicy(TArray<UDSMDefaultNode*> transitionableNodes, const TWeakObjectPtr<ADSMGameMode> gameMode, bool bSelfTransition /*= false*/)
{
	bool eventSuccess = false;
	bool success = false;
	TArray<UDSMDefaultNode*> eventNodes;
	ApplyPolicyEvent(transitionableNodes, gameMode.Get(), eventNodes, eventSuccess, bSelfTransition);
	TArray<UDSMDefaultNode*> nodes = ApplyPolicy(transitionableNodes, gameMode, success, bSelfTransition);
	// You can either implement policy in BP or C++, but not in both
	if ((eventNodes.Num() > 0 && nodes.Num() > 0) || (eventSuccess && success))
	{
		UE_LOG(LogDSM, Error, TEXT("Apply policy can be called in BP or in C++ but never in both !!!"));
		check(false);
		return;
	}
	nodes.Append(eventNodes);
	_policyNodesOrdered = nodes;
	bAppliedSuccessful = eventSuccess || success;
}

bool UDSMPolicy::HasPolicyFinished() const
{
	return _policyNodesOrdered.Num() == 0;
}

UDSMDefaultNode* UDSMPolicy::HandleNextNode()
{
	if (!HasPolicyFinished())
	{
		UDSMDefaultNode* nextNode = _policyNodesOrdered[0];
		_policyNodesOrdered.RemoveAt(0);
		return nextNode;
	}
	return nullptr;
}


TArray<UDSMDefaultNode*> UDSMDefaultPolicy::ApplyPolicy(const TArray<UDSMDefaultNode*>& inputNodes, const TWeakObjectPtr<ADSMGameMode> gameMode, bool& bSuccess, bool bSelfTransition /*= false*/) const
{
	TMap<FString, FDSMDebugConditions> SuccessfulNodes;
	TMap<FString, FDSMDebugConditions> UnsuccessfulNodes;
	
	TArray<UDSMDefaultNode*> transitionNodes = {};
	for (const TWeakObjectPtr<UDSMDefaultNode> node : inputNodes)
	{
		TTuple<FString, FDSMDebugConditions> debugInfo;
		if (node->EvaluateEnterConditions(bSelfTransition, debugInfo))
		{
			SuccessfulNodes.Add(debugInfo);
			transitionNodes.Add(node.Get());
		}
		else
		{
			UnsuccessfulNodes.Add(debugInfo);
		}
	}
	// Add debug info
	float realtimeSeconds = UGameplayStatics::GetRealTimeSeconds(GetWorld());
	gameMode->_stateMachineDebugData.Add({SuccessfulNodes, UnsuccessfulNodes, realtimeSeconds});

	// This policy only allows a single return node or nothing, otherwise no success
	switch (transitionNodes.Num())
	{
	case 0:
	{
		UE_LOG(LogDSM, Log, TEXT("Policy %s could not find a transition to a next node"), *GetName());
		bSuccess = false;
		break;
	}

	case 1:
	{
		UE_LOG(LogDSM, Log, TEXT("Policy found applicable node %s (outer : %s)"),
			*transitionNodes[0]->GetName(),
			*transitionNodes[0]->GetOuter()->GetName());
		bSuccess = true;
		break;
	}
	default:
	{
		UE_LOG(LogDSM, Log, TEXT("Policy %s found %d applicable nodes"), *GetName(), transitionNodes.Num());
		bSuccess = false;
		break;
	}
	}
	return transitionNodes;
}

TArray<UDSMDefaultNode*> UDSMOrderByTagPolicy::ApplyPolicy(const TArray<UDSMDefaultNode*>& inputNodes, const TWeakObjectPtr<ADSMGameMode> gameMode, bool& bSuccess, bool bSelfTransition) const
{
	TArray<UDSMDefaultNode*> outputNodes;

	for (const FName& tag : TagOrder)
	{
		outputNodes.Append(inputNodes.FilterByPredicate([tag](const UDSMDefaultNode* node) { return node->ComponentHasTag(tag); }));
	}
	if (outputNodes.Num() > 0)
	{
		bSuccess = true;
	}
	else
	{
		bSuccess = false;
	}
	return outputNodes;
}
