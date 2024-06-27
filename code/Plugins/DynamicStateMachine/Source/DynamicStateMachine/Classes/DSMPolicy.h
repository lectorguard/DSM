// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "DSMPolicy.generated.h"

class UDSMDefaultNode;
class ADSMGameMode;


/**
 * Each DSM node can have one or more policies
 * A policy basically resolves the conflict, if one or more nodes can become active at the same time
 * A policy can get applied, only if all considered nodes support the policy
 * A policy can succeed or fail with one or mulitple nodes.
 * Based on the outputed nodes, the system always tries to find a policy with a higher priority
 * We choose the policy which is successful and has the highest priority.
 */
UCLASS(Blueprintable)
class DYNAMICSTATEMACHINE_API UDSMPolicy : public UActorComponent
{
	GENERATED_BODY()

public:

	// If all considered nodes support this policy, the policy is applied
	// inputNodes are the considered nodes which support this policy
	// gameMode is the DSM game mode in the current level
	// orderedOutputNodes is a subset of the input node in the order the nodes should get executed
	// orderedOutputNodes can be the filtered inputNodes, based on the policy
	// orderedOutputNodes based on these nodes, the system will try to apply more applicable nodes
	// bSuccess if set to true the policy is applied successful
	// bSuccess if set to false, policy will not become active 
	// bSelfTransition if true, we try to find a policy because a DSM node wants to transition to itself
	UFUNCTION(BlueprintImplementableEvent, Category = "Dynamic State Machine")
	void ApplyPolicyEvent(const TArray<UDSMDefaultNode*>& inputNodes, const ADSMGameMode* gameMode, TArray<UDSMDefaultNode*>& orderedOutputNodes, bool& bSuccess, bool bSelfTransition = false);
	virtual TArray<UDSMDefaultNode*> ApplyPolicy(const TArray<UDSMDefaultNode*>& inputNodes, const TWeakObjectPtr<ADSMGameMode> gameMode,  bool& bSuccess, bool bSelfTransition = false) const { return {}; };

	// Helper function to find the first element in an object array which is of a certain class
	UFUNCTION(BlueprintCallable, Category = "Dynamic State Machine", meta = (DeterminesOutputType = "assetType"))
	UObject* FindFirstElementByClass(const TArray<UObject*>& inputObjects, TSubclassOf<UObject> assetType) const;

	// Helper function which filters out all elements which are not of type filterType
	UFUNCTION(BlueprintCallable, Category = "Dynamic State Machine", meta = (DeterminesOutputType = "filterType"))
	TArray<UObject*> FilterByClass(const TArray<UObject*>& inputObjects, TSubclassOf<UObject> filterType) const;
	
	// Activates this policy, calls ApplyPolicy BP event and C++ function
	void ActivatePolicy(TArray<UDSMDefaultNode*> transitionableNodes, const TWeakObjectPtr<ADSMGameMode> gameMode, bool bSelfTransition = false);
	
	// Returns the next node in this policy, nullptr if there is no next node
	UDSMDefaultNode* HandleNextNode();

	// Checks if there are any remaining nodes in this policy
	bool HasPolicyFinished() const;

	// Returns true, if there should be a transition after this policy
	bool TransitionAfterPolicy() const { return bTransitionAfterPolicy; }

	// Returns the priority of this node
	const int32 GetPriority() const { return _priority; }

	// Returns all DSM nodes currently associated with this node 
	const TArray<UDSMDefaultNode*> GetPolicyNodes() const { return _policyNodesOrdered; }

	// Returns true if this policy is successful
	const bool IsPolicyAppliedSuccesfully() const { return bAppliedSuccessful; }

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Dynamic State Machine")
	int32 _priority = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Dynamic State Machine")
	bool bTransitionAfterPolicy = true;

private:
	bool bAppliedSuccessful = false;
	TArray<UDSMDefaultNode*> _policyNodesOrdered = {};
};


/**
 * Default policy for each DSM node
 * Transition to the node which suffices the enter condition
 * If multiple nodes suffice the enter condition, the policy fails
 * System will check on another policy for the nodes with the successful enter condition
 */
UCLASS(BlueprintType)
class DYNAMICSTATEMACHINE_API UDSMDefaultPolicy : public UDSMPolicy
{
	GENERATED_BODY()

public:
	UDSMDefaultPolicy()
	{
		_priority = 0;
	}

	TArray<UDSMDefaultNode*> ApplyPolicy(const TArray<UDSMDefaultNode*>& inputNodes, const TWeakObjectPtr<ADSMGameMode> gameMode, bool& bSuccess, bool bSelfTransition = false) const override;
};

/**
 * Looks for DSM nodes with tags
 * If one or more nodes with the predefined tags are found, the policy is successful
 * Found nodes are executed based on tag order
 */
UCLASS(BlueprintType)
class DYNAMICSTATEMACHINE_API UDSMOrderByTagPolicy : public UDSMPolicy
{
	GENERATED_BODY()

public:
	UDSMOrderByTagPolicy()
	{
		_priority = 10;
	}

	// Policy filters for nodes with tags in the list
	// Nodes are executed based on tag order
	UPROPERTY(EditAnywhere, Category = "Tags")
	TArray<FName> TagOrder = {};

	TArray<UDSMDefaultNode*> ApplyPolicy(const TArray<UDSMDefaultNode*>& inputNodes, const TWeakObjectPtr<ADSMGameMode> gameMode, bool& bSuccess, bool bSelfTransition = false) const override;

};




