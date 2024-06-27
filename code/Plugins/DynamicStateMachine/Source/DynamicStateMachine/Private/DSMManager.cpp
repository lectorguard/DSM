// Fill out your copyright notice in the Description page of Project Settings.


#include "DSMManager.h"
#include "DSMLogInclude.h"
#include "Algo/Transform.h"
#include "Kismet/GameplayStatics.h"
#include "DSMPolicy.h"
#include "TimerManager.h"


ADSMGameMode::SaveLoadInfo ADSMGameMode::_saveLoadInfo = { "", true };

ADSMGameMode::ADSMGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	_stateMachineData = CreateDefaultSubobject<UDSMSaveGame>(TEXT("DSM Data"));
}

void ADSMGameMode::BeginPlay()
{
	Super::BeginPlay();
	// State machine starts automatically after all DefaultNodes registered at the manager, PostInitializeComponents is called after BeginPlay
	GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
		{	
			StartStateMachine();
		});
}

void ADSMGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateStateMachine(DeltaSeconds);
}

void ADSMGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	StopStateMachine();
}

void ADSMGameMode::StartStateMachine()
{
	LoadSaveGame_Internal(_saveLoadInfo);
	// Clear Save Load Info after load process
	_saveLoadInfo = SaveLoadInfo();
	if (bRequestTransitionAfterBeginPlay)
	{
		TransitionState();
	}
}



void ADSMGameMode::StopStateMachine()
{
	if (IsValid(_currentNode))
	{
		EndState();
		_currentNode = nullptr;
	}
	_defaultNodes.Empty();
	_hasStateEnded = false;
}

void ADSMGameMode::BeginState(TWeakObjectPtr<UDSMDefaultNode> node)
{
	check(node.IsValid() && "Valid node must be passed");
	
	
	_currentNode = UDSMActiveNode::Create(node.Get());
	if(IsValid(_currentNode->_node))_currentNode->_node->InitNode();
	if (IsValid(_currentNode->_node))_currentNode->_node->InitNodeEvent();
	bool blocalHasStateEnded = false;
	bool blocalHasStateEndedEvent = false;
	if (IsValid(_currentNode->_node))_currentNode->_node->OnBeginState(blocalHasStateEnded);
	if (IsValid(_currentNode->_node))_currentNode->_node->OnBeginStateEvent(blocalHasStateEndedEvent);
	if (IsValid(_currentNode->_node))_currentNode->_node->ApplyStateBegin();
	if (IsValid(_currentNode->_node))_currentNode->_node->ApplyStateBeginEvent();
	_hasStateEnded = blocalHasStateEnded || blocalHasStateEndedEvent;

	UE_LOG(LogDSM, Log, TEXT("DSM State Info : Begin state %s"),
		*(node.IsValid() ? node->GetName(): FString("node")));
	
}

void ADSMGameMode::TransitionState()
{
	if (!_IsTransitionAllowed)
	{
		return;
	}

	// Finishes current state
	EndState();

	// If there is no policy we need to find one, if _currentPolicy disallows transtion we skip this part
	const bool bTransitionAfterPolicy = _currentPolicy ? _currentPolicy->TransitionAfterPolicy() : true;
	const bool bNeedNewPolicy = (!_currentPolicy || _currentPolicy->HasPolicyFinished()) && bTransitionAfterPolicy;
	if (bNeedNewPolicy)
	{
		bool bSuccess = false;
		TObjectPtr<UDSMPolicy> foundPolicy = FindPolicy(_defaultNodes, bSuccess);
		if (bSuccess)
		{
			_currentPolicy = foundPolicy;
		}
		else
		{
			UE_LOG(LogDSM, Log, TEXT("DSM transition chain has ended, active node is empty"));
			_currentPolicy = nullptr;
			_currentNode = nullptr;
			return;
		}
	}

	if (!_currentPolicy->HasPolicyFinished())
	{
		TObjectPtr<UDSMDefaultNode> nextNode = _currentPolicy->HandleNextNode();
		if (!IsValid(nextNode))
		{
			_currentPolicy = nullptr;
			_currentNode = nullptr;
			return;
		}
		UE_LOG(LogDSM, Log, TEXT("DSM policy transition : next node is %s (outer : %s ), policy %s"),
			*nextNode->GetName(),
			*nextNode->GetOuter()->GetName(),
			*_currentPolicy->GetName());
		BeginState(nextNode);
	}
	else
	{
		if (!bTransitionAfterPolicy)
		{
			UE_LOG(LogDSM, Log, TEXT("Policy %s was found successfully, but policy does not want to perform transition."),
				*_currentPolicy->GetName());
		}
		else
		{
			UE_LOG(LogDSM, Warning, TEXT("Policy %s was found successfully, but there is no node to handle. A policy should be only successful if there is at least  node to transition to"),
				*_currentPolicy->GetName());
		}
		
		_currentPolicy = nullptr;
		_currentNode = nullptr;
	}
}

const TObjectPtr<UDSMPolicy> ADSMGameMode::FindPolicy(const TArray<UDSMDefaultNode*> transitionNodes, bool& bSuccess, bool bSelfTransition /*= false*/, TArray<TObjectPtr<UDSMPolicy>> appliedPolicies) {
	// Otherwise there is an issue
	if (transitionNodes.Num() > 0)
	{
		TArray<TSubclassOf<UDSMPolicy>> applicablePolicies = transitionNodes[0]->_nodePolicies;
		for (const TWeakObjectPtr<UDSMDefaultNode> node : transitionNodes)
		{
			TArray<TSubclassOf<UDSMPolicy>> newApplicablePolicies = {};
			for (const TSubclassOf<UDSMPolicy> policy : node->_nodePolicies)
			{
				
				if (policy && applicablePolicies.Contains(policy))
				{
					newApplicablePolicies.Add(policy);
				}
			}
			if (newApplicablePolicies.IsEmpty())
			{
				bSuccess = false;
				return {};
			}
			applicablePolicies = newApplicablePolicies;
		}
		// Never apply same policy twice
		for (const TObjectPtr<UDSMPolicy> policyInstance : appliedPolicies)
		{
			applicablePolicies.Remove(policyInstance->GetClass());
		}
		if (applicablePolicies.Num() > 0)
		{
			TArray<TObjectPtr<UDSMPolicy>> applicablePoliciesInstances = {};
			for (const TSubclassOf<UDSMPolicy>& policy : applicablePolicies)
			{
				applicablePoliciesInstances.Add(NewObject<UDSMPolicy>(this, policy));
			}
			//Ascending sorting
			applicablePoliciesInstances.Sort([](const UDSMPolicy& R, const UDSMPolicy& L)
				{
					return R.GetPriority() < L.GetPriority();
				});
			// Activate the best found policy, based on result try to activate the next policy
			TObjectPtr<UDSMPolicy> policy = DuplicateObject<UDSMPolicy>(applicablePoliciesInstances.Last(), this);
			policy->ActivatePolicy(transitionNodes, this, bSelfTransition);
			UE_LOG(LogDSM, Log, TEXT("Policy %s was activated %s with %d nodes, based on result try to find policy with higher priority"),
				*policy->GetName(),
				*(policy->IsPolicyAppliedSuccesfully() ? FString("successfully") : FString("unsuccessful")),
				policy->GetPolicyNodes().Num());
			for (const UDSMDefaultNode* defaultNode : policy->GetPolicyNodes())
			{
				UE_LOG(LogDSM, Log, TEXT("Found node : %s (outer : %s)"),
					*defaultNode->GetName(),
					*defaultNode->GetOuter()->GetName());
			}
			UE_LOG(LogDSM, Log, TEXT("-------------------------------------------"));
			appliedPolicies.Add(policy);
			return FindPolicy(policy->GetPolicyNodes(), bSuccess, bSelfTransition, appliedPolicies);
		}
	}
	// From all found policies find the best policy which is successful and has highest score
	TArray<TObjectPtr<UDSMPolicy>> succesfuleElements = appliedPolicies.FilterByPredicate([](const TObjectPtr<UDSMPolicy> policy) { return policy->IsPolicyAppliedSuccesfully(); });
	succesfuleElements.Sort([](const UDSMPolicy& R, const UDSMPolicy& L) { return R.GetPriority() < L.GetPriority(); });
	if (succesfuleElements.Num() > 0)
	{
		UE_LOG(LogDSM, Log, TEXT("Best successful policy found %s with %d nodes"),
			*succesfuleElements.Last()->GetName(),
			succesfuleElements.Last()->GetPolicyNodes().Num());
		bSuccess = true;
		return succesfuleElements.Last();
	}
	else
	{
		UE_LOG(LogDSM, Log, TEXT("Could not find any successful node."));
		bSuccess = false;
		return nullptr;
	}
}

void ADSMGameMode::UpdateStateMachine(float DeltaTime)
{
	// For each update there is just a single begin state, or update state. End State can not be called in the same frame as begin or update state.
	if (_hasStateEnded)
	{
		TransitionState();
	}
	else if (IsValid(_currentNode))
	{

		bool blocalHasStateEnded = false;
		bool blocalHasStateEndedEvent = false;
		if (IsValid(_currentNode->_node))_currentNode->_node->OnUpdateState(DeltaTime, blocalHasStateEnded);
		if (IsValid(_currentNode->_node))_currentNode->_node->OnUpdateStateEvent(DeltaTime, blocalHasStateEndedEvent);
		if (IsValid(_currentNode->_node))_currentNode->_node->ApplyStateUpdate();
		if (IsValid(_currentNode->_node))_currentNode->_node->ApplyStateUpdateEvent();
		_hasStateEnded = blocalHasStateEnded || blocalHasStateEndedEvent;
	}
}

void ADSMGameMode::EndState()
{
	if (IsValid(_currentNode) && _stateMachineData)
	{
		if (IsValid(_currentNode->_node))_currentNode->_node->OnEndState();
		if (IsValid(_currentNode->_node))_currentNode->_node->OnEndStateEvent();
		if (IsValid(_currentNode->_node))_currentNode->_node->ApplyStateEnd();
		if (IsValid(_currentNode->_node))_currentNode->_node->ApplyStateEndEvent();
		_stateMachineData->AddMemory(_currentNode->_node,_currentNode->_cachedReferences);
		_hasStateEnded = false;

		UE_LOG(LogDSM, Log, TEXT("DSM State Info : End state %s"), *_currentNode->_node->GetName());
	}
}

bool ADSMGameMode::RequestCustomTransition(TWeakObjectPtr<UDSMDefaultNode> node)
{
	if (!IsActive())
	{
		bool bSuccess = false;
		TObjectPtr<UDSMPolicy> foundPolicy = FindPolicy({node.Get()}, bSuccess, true);
		if (bSuccess)
		{
			_currentPolicy = foundPolicy;
			TransitionState();
			return true;
		}	
		UE_LOG(LogDSM, Log, TEXT("Node %s (outer : %s) is not valid for any assigned policy"),
			*(node.IsValid() ? node->GetName() : FString("None")),
			*(node.IsValid() ? node->GetOuter()->GetName() : FString("None")));
	}
	return false;
}

void ADSMGameMode::RegisterNode(UDSMDefaultNode* defaultNode)
{
	check(defaultNode);
	UWorld* world = defaultNode->GetWorld();
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(world, ADSMGameMode::StaticClass(), FoundActors);
	if (FoundActors.Num() == 1)
	{
		if (ADSMGameMode* manager = Cast<ADSMGameMode>(FoundActors[0]))
		{
			if (!manager->_defaultNodes.Contains(defaultNode))
			{
				defaultNode->SetDSMManager(manager, [manager](TWeakObjectPtr<UDSMDefaultNode> node) 
					{
						return manager->RequestCustomTransition(node); 
					});
				manager->_defaultNodes.Add(defaultNode);
			}
			else
			{
				UE_LOG(LogDSM, Error, TEXT("DSM Manager already contains DefaultNode %s with address %d"), *defaultNode->GetName(), &defaultNode);
			}
		}
	}
	else if (FoundActors.Num() > 1)
	{
		UE_LOG(LogDSM, Error, TEXT("There is more than one DSM Manager in level %s, only one manager can be in a level"), *world->GetName());
	}
	else
	{
		UE_LOG(LogDSM, Error, TEXT("DSM manager is missing in level %s, please add DSM Manager Actor to the level"), *world->GetName());
	}
}

bool ADSMGameMode::UnregisterNode(UDSMDefaultNode* nodeToUnregister)
{
	check(nodeToUnregister);
	UWorld* world = nodeToUnregister->GetWorld();

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(world, ADSMGameMode::StaticClass(), FoundActors);
	if (FoundActors.Num() == 1)
	{
		if (ADSMGameMode* manager = Cast<ADSMGameMode>(FoundActors[0]))
		{
			int32 removedElements = manager->_defaultNodes.Remove(nodeToUnregister);
			return removedElements > 0;
		}
	}
	else if (FoundActors.Num() > 1)
	{
		UE_LOG(LogDSM, Error, TEXT("There is more than one DSM Manager in level %s, only one manager can be in a level"), *world->GetName());
	}
	else
	{
		UE_LOG(LogDSM, Error, TEXT("DSM manager is missing in level %s, please add DSM Manager Actor to the level"), *world->GetName());
	}
	return false;
}

bool ADSMGameMode::RequestTransition_Internal()
{
	if (!_hasStateEnded && !IsValid(_currentNode))
	{
		TransitionState();
		return true;
	}
	return false;
}

bool ADSMGameMode::RequestDSMTransition(const UObject* worldContext)
{
	const UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::LogAndReturnNull);
	return RequestDSMTransition(world);
}

bool ADSMGameMode::RequestDSMTransition(const UWorld* world)
{
	check(world);
	if (ADSMGameMode* dsmGameMode = Cast<ADSMGameMode>(world->GetAuthGameMode()))
	{
		return dsmGameMode->RequestTransition_Internal();
	}
	else
	{
		UE_LOG(LogDSM, Error, TEXT("DSM can only be used if a game mode inherits from ADSMGameMode is used. Current game mode %s does not support DSM"), *world->GetAuthGameMode()->GetName());
	}
	return false;
}

void ADSMGameMode::LoadSaveGame_Internal(const SaveLoadInfo saveLoadInfo)
{
	if (saveLoadInfo._saveSlotName.IsEmpty())
	{
		return;
	}

	// Prevent any node from performing unpredictable transitions
	_IsTransitionAllowed = false;
	TObjectPtr<UDSMSaveGame> loadedSaveGame = Cast<UDSMSaveGame>(UGameplayStatics::LoadGameFromSlot(_saveLoadInfo._saveSlotName, 0));
	if (loadedSaveGame)
	{
		loadedSaveGame->PostDeserialization(GetWorld(), _saveLoadInfo._saveSlotName);
		if (saveLoadInfo._deleteSlotAfterLoad)
		{
			// Reset slot, so we do not always load the default save game
			UGameplayStatics::DeleteGameInSlot(_saveLoadInfo._saveSlotName, 0);
		}
		// Keep entire state, nodes are applied based on the general progress
		_stateMachineData->SetStateMachineHistory({});
		const TArray<FDSMNodeID> loadedSaveGameHistory = loadedSaveGame->GetStateMachineHistory();
		TArray<TObjectPtr<AActor>> actorCache = {};
		for (int32 i = 0; i < loadedSaveGame->_indexToLoad + 1; ++i)
		{
			const FDSMNodeID& node = loadedSaveGameHistory[i];
			_stateMachineData->PushStateMachineElement(node);
			TWeakObjectPtr<UDSMDefaultNode> foundNode = GetComponentFromNodeID(node, actorCache);
			if (foundNode.IsValid())
			{
				// Allow node to create variables, btw. cache some information
				_currentNode = UDSMActiveNode::Create(foundNode.Get());
				_currentPolicy = nullptr;
				// Apply all states, nodes can be destroyed at all time 
				if (foundNode.IsValid())foundNode->ApplyStateBegin();
				if (foundNode.IsValid())foundNode->ApplyStateBeginEvent();
				if (foundNode.IsValid())foundNode->ApplyStateUpdate();
				if (foundNode.IsValid())foundNode->ApplyStateUpdateEvent();
				if (foundNode.IsValid())foundNode->ApplyStateEnd();
				if (foundNode.IsValid())foundNode->ApplyStateEndEvent();
				_currentPolicy = nullptr;
				_currentNode = nullptr;
			}
			else
			{
				UE_LOG(LogDSM, Error, TEXT("Could not load save game node %s outer %s, node could not be found"), *node._nodeLabel.ToString(), *node._ownerLabel.ToString());
				return;
			}
		}
		if (loadedSaveGame->_keepState)
		{
			_stateMachineData->SetStateMachineHistory(loadedSaveGame->GetStateMachineHistory());
		}
	}
	_currentPolicy = nullptr;
	_currentNode = nullptr;
	_IsTransitionAllowed = true;
}
TWeakObjectPtr<UDSMDataAsset> ADSMGameMode::GetDataAssetCached(const TWeakObjectPtr<UDSMDataAsset> DefaultDataAssetObject)
{
	if (_stateMachineData)
	{
		// Caching only used if active node, otherwise always a copy is returned
		if (IsValid(_currentNode))
		{
			const FName defaultName = DefaultDataAssetObject->GetFName();
			if (!_currentNode->_cachedReferences.Contains(defaultName))
			{
				_currentNode->_cachedReferences.Add(defaultName, _stateMachineData->GetDataCopy(DefaultDataAssetObject));
			}
			return _currentNode->_cachedReferences[defaultName];
		}
		// if current node is invalid access can only be read only
		return _stateMachineData->GetDataCopy(DefaultDataAssetObject);
	}
	UE_LOG(LogDSM, Error, TEXT("State Machine Data is invalid"));
	return nullptr;
}


TWeakObjectPtr<UDSMDefaultNode> ADSMGameMode::GetComponentFromNodeID(const FDSMNodeID& node, TArray<TObjectPtr<AActor>>& cachedActors)
{
	// If node valid, we simply return the node
	if (node._node.IsValid())
	{
		return node._node;
	}
	// Node was created dynamically, we need to search for the owning actor first
	TObjectPtr<AActor> referencedActor = node._owner.Get();
	if (!referencedActor)
	{
		// Check if we already found the actor
		TObjectPtr<AActor>* foundCached = cachedActors.FindByPredicate([name = node._ownerLabel](TObjectPtr<AActor> other)
			{
				return other->GetFName() == name;
			});
		if (foundCached)
		{
			referencedActor = *foundCached;
		}
		// Search for actor by hand
		else
		{
			TArray<TObjectPtr<AActor>> FoundActors;
			UGameplayStatics::GetAllActorsOfClass(GetWorld(), node._ownerClass, FoundActors);
			TObjectPtr<AActor>* foundCorrect = FoundActors.FindByPredicate([name = node._ownerLabel](TObjectPtr<AActor> other)
				{
					return other->GetFName() == name;
				});
			if (foundCorrect)
			{
				referencedActor = *foundCorrect;
				cachedActors.Add(referencedActor);
			}
			else
			{
				UE_LOG(LogDSM, Error, TEXT("Can not find actor with name %s, loading state failed."), *node._ownerLabel.ToString());
				return nullptr;
			}
		}
	}
	// Search for the component with this name
	TArray<UActorComponent*> foundComponents;
	referencedActor->GetComponents(node._nodeClass, foundComponents);
	UActorComponent** targetComp = foundComponents.FindByPredicate([name = node._nodeLabel](UActorComponent* other)
		{
			return other->GetFName() == name;
		});
	if (targetComp)
	{
		TWeakObjectPtr<UDSMDefaultNode> found = Cast<UDSMDefaultNode>(*targetComp);
		return found;
	}
	else
	{
		UE_LOG(LogDSM, Error, TEXT("Can not find component with name %s (outer actor %s), loading state failed."), *node._nodeLabel.ToString(), *referencedActor->GetName());
		return nullptr;
	}
}


