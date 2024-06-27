// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DSMSaveGame.h"
#include "DSMDefaultNode.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/DataAsset.h"
#include "Misc/Optional.h"
#include "DSMManager.generated.h"

/*
* Class defining the currently active node.
*/
UCLASS(BlueprintType)
class DYNAMICSTATEMACHINE_API UDSMActiveNode : public UObject
{
	GENERATED_BODY()

public:
	// Node which is currently active
	UPROPERTY()
	TObjectPtr<UDSMDefaultNode> _node = nullptr;

	// Cached data references associated to the currently active node
	// After state ends, _cachedReferences are written to the state history (saveGame)
	UPROPERTY()
	TMap<FName, TObjectPtr<UDSMDataAsset>> _cachedReferences = {};

	// Helper method to create new active node
	static TObjectPtr<UDSMActiveNode> Create(TObjectPtr<UDSMDefaultNode> object)
	{
		TObjectPtr<UDSMActiveNode> node = NewObject<UDSMActiveNode>();
		node->_node = object;
		node->_cachedReferences = {};
		return node;
	}
};

/**
 * Every level managed by DSM needs a ADSMManager actor placed in the level
 * State machine will start as soon as the registration process of the DSMNodes is finished (Registration happens during Event BeginPlay())
 * This class manages the DSM system, including handling transitions and save/load of the state machine history
 */
UCLASS(Blueprintable)
class DYNAMICSTATEMACHINE_API ADSMGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ADSMGameMode();

	// Holds save game information and history of the state machine
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Dynamic State Machine")
	TObjectPtr<UDSMSaveGame> _stateMachineData = nullptr;

	// Holds debug information about all transition attempts
	UPROPERTY(VisibleAnywhere, Category = "Dynamic State Machine")
	TArray<FDSMDebugSuccess> _stateMachineDebugData = {};

	// Stops the state machine
	// Ends current active state by force
	UFUNCTION(BlueprintCallable, Category = "Dynamic State Machine")
	virtual void StopStateMachine();

	// Checks if state machine has an active node
	// State machine transitions between nodes, 
	// if no new next node can be found state machine idles (not active)
	UFUNCTION(BlueprintCallable, Category = "Dynamic State Machine")
	bool IsActive() { return IsValid(_currentNode) && _currentPolicy; }

	// Returns all DSM nodes registered to this DSM game mode
	// DSM nodes in the world register her by themself when getting spawned or at begin play
	UFUNCTION(BlueprintCallable, Category = "Dynamic State Machine")
	TArray<UDSMDefaultNode*> GetAvailableNodes() { return _defaultNodes; }

	// Returns currently active node, or nullptr
	UFUNCTION(BlueprintCallable, Category = "Dynamic State Machine")
	UDSMDefaultNode* GetActiveNode() { return IsValid(_currentNode) ? _currentNode->_node : nullptr; }

	// Registers a UDSM Default Node
	// Called automatically on spawn and on begin play on each default node
	static void RegisterNode(UDSMDefaultNode* defaultNode);

	// Unregisters a UDSM Default Node
	// Called automatically on destroy and on end play on each default node
	static bool UnregisterNode(UDSMDefaultNode* nodeToUnregister);

	// Requests dynamic state machine to perform a transition, only possible if there is no active node
	// If a transition is found and found nodes end, another transition is requested automatically
	// Returns true if transition can be performed
	// Returns false otherwise
	UFUNCTION(BlueprintCallable, Category = "Dynamic State Machine", meta = (WorldContext = "worldContext"))
	static bool RequestDSMTransition(const UObject* worldContext);
	static bool RequestDSMTransition(const UWorld* world);

	// Triggers automatically a transition after DSM node registration process has finished
	UPROPERTY(EditAnywhere, Category = "Dynamic State Machine")
	bool bRequestTransitionAfterBeginPlay = false;

	// Returns latest version of a data reference, based on the history and current active node
	// If there is no current active node, latest version is searched in history
	TWeakObjectPtr<UDSMDataAsset> GetDataAssetCached(const TWeakObjectPtr<UDSMDataAsset> DefaultDataAssetObject);

protected:
	// Holds all currently registered DSM nodes
	UPROPERTY(VisibleAnywhere , Category = "Dynamic State Machine")
	TArray<TObjectPtr<UDSMDefaultNode>> _defaultNodes = {};

	// Startup and shutdown
	void BeginPlay() override;
	void Tick(float DeltaSeconds) override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// State machine is started automatically after initialization phase
	void StartStateMachine();

	// Performs a transition if possible
	bool RequestTransition_Internal();

	// Handling transitions between nodes 
	void BeginState(TWeakObjectPtr<UDSMDefaultNode> node);
	
	// Updates the state machine
	virtual void UpdateStateMachine(float DeltaTime);
	
	// Requests a transition internally
	void TransitionState();

	// Finds a new policy which is applicable to the current registered DSM nodes and data references
	// Different policies can get activated to filter for valid transition nodes and to find policies with a high priority
	const TObjectPtr<UDSMPolicy> FindPolicy(const TArray<UDSMDefaultNode*> transitionNodes, bool& bSuccess, bool bSelfTransition = false, TArray<TObjectPtr<UDSMPolicy>> appliedPolicies = {});

	// Ends a current state
	void EndState();

	// Custom transition can only get called from the owning default node
	bool RequestCustomTransition(TWeakObjectPtr<UDSMDefaultNode> node);

private:
	TWeakObjectPtr<UDSMDefaultNode> GetComponentFromNodeID(const FDSMNodeID& node, TArray<TObjectPtr<AActor>>& cachedActors);

	// Holds currently active node
	UPROPERTY()
	TObjectPtr<UDSMActiveNode> _currentNode = nullptr;
	
	// Holds currently active policy
	// Policy can define a sequence of nodes executed next
	// If sequence ends, policy becomes invalid and a new policy must be found
	UPROPERTY()
	TObjectPtr<class UDSMPolicy> _currentPolicy = nullptr;

	// Status of current active node
	bool _hasStateEnded = false;
	bool _IsTransitionAllowed = true;

public:
	// Save load information
	struct SaveLoadInfo
	{
		FString _saveSlotName{};
		bool _deleteSlotAfterLoad = true;
	};
	// Load info must be static to survive level transitions
	static SaveLoadInfo _saveLoadInfo;
private:

	// Applies the new save game
	void LoadSaveGame_Internal(const SaveLoadInfo saveLoadInfo);
public:
	// Sets save laod information
	static void SetSaveLoadInfo(const SaveLoadInfo& saveLoadInfo) { _saveLoadInfo = saveLoadInfo; }
};


