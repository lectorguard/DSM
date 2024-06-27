// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "DSMConditionUtils.h"
#include "DSMDefaultNode.generated.h"


/*
* Debug information about successful and unsuccessful conditions of a DSM node
*/
USTRUCT(BlueprintType)
struct DYNAMICSTATEMACHINE_API FDSMDebugConditions
{
	GENERATED_BODY()

	// Key is the condition name defined for the DSM node
	// Value shows if condition is true or false
	UPROPERTY(VisibleAnywhere, Category = "Conditions")
	TMap<FName, bool> Conditions;
};

/*
* Debug information about successful and unsuccessful DSM nodes
*/
USTRUCT(BlueprintType)
struct DYNAMICSTATEMACHINE_API FDSMDebugSuccess
{
	GENERATED_BODY()

	// Key is the name of the DSM node
	// Value list all DSM nodes which are enterable
	UPROPERTY(VisibleAnywhere, Category = "NodeSuccess")
	TMap<FString, FDSMDebugConditions> Successful;

	// Key is the name of the DSM node
	// Value list all DSM nodes which are NOT enterable
	UPROPERTY(VisibleAnywhere, Category = "NodeSuccess")
	TMap<FString, FDSMDebugConditions> Unsuccessful;

	// Timestamp when node transition was requested
	// If 2 or more FDSMDebugSuccess elements have small delta, they are probably executed by the same DSM policy
	UPROPERTY(VisibleAnywhere, Category = "NodeSuccess")
	float ElapsedTime = 0.0f;
};


/**
 * DSM Node. 
 * Each state in DSM is defined by a UDSMDefaultNode actor component. All nodes are manage by the ADSMGameMode.
 * A state only contains behavior and references to data. It never stores any data by itself. 
 * If a node gets active, it can update data references based on the defined behavior in the node and apply the data state changes to the world.
 */
UCLASS(Blueprintable)
class DYNAMICSTATEMACHINE_API UDSMDefaultNode : public UActorComponent
{
	GENERATED_BODY()
public:

	UDSMDefaultNode();

	// Applicable policies for this node
	UPROPERTY(EditDefaultsOnly, Category="Policy")
	TArray<TSubclassOf<class UDSMPolicy>> _nodePolicies = {};

	// Data references this node can write to during state updates
	// DSM keeps track of mutation of writable data references
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Data")
	TMap<FName, TObjectPtr<UDSMDataAsset>> _writableDataReferences = {};

	// Data references which are read only for this node
	// History is not stored
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Data")
	TMap<FName, TObjectPtr<UDSMDataAsset>> _readOnlyDataReferences = {};

	// Define own condition objects, which bind properties to a name
	// You can use these bindings inside condition groups to define bool expressions under which this state can become active
	// You can create custom condition objects in C++ and expose them to the editor when inheriting from UDSMConditionBase. Check out implementations of this class as a reference
	UPROPERTY(EditAnywhere, Instanced, Category = "Conditions")
	TMap<FName, TObjectPtr<class UDSMConditionBase>> _ConditionDefinitions = {};

	// Condition group define conditions expressions under which this node can become active using the DefaultPolicy
	// Conditions defined here are ignored if the DefaultPolicy is not used
	// You can use condition groups in combination with the CanEnterStateEvent BlueprintEvent and the CanEnterState C++ function
	// All entries are &&ed during evaluation
	UPROPERTY(EditAnywhere, Category = "Conditions",  meta = (MultiLine = true))
	TMap<FName, FText> _ConditionGroups = {};

	// This flag is set, if all the condition groups are valid/working
	// If this flag can not be set to true look at the outputLog
	// Probably you have a typo or there is another issue
	// Untick this flag to update Condition Definitions
	UPROPERTY(EditDefaultsOnly, Category = "Conditions")
	bool _BindConditions = false;

	// Event fired when node is intialized
	UFUNCTION(BlueprintImplementableEvent, Category = "Dynamic State Machine")
	void InitNodeEvent();
	virtual void InitNode() {};

	// Event fired during transition process, when trying to find a new node
	// During this event, access to data references is always read-only
	// bIsSelfTransitioned flag is set if the transition was self triggered
	// CanEnter key is the description of a condition which has to be satisfied in this node to become active
	// CanEnter value is the calculated bool value of the current condtion description
	UFUNCTION(BlueprintImplementableEvent, Category = "Dynamic State Machine")
	void CanEnterStateEvent(bool bIsSelfTransitioned, TMap<FName, bool>& CanEnter) const;
	virtual void CanEnterState(bool bIsSelfTransitioned, TMap<FName, bool>& CanEnter) const {};

	// Event fired when transitioning to this node
	// Inside this node you should update data-references based on the defined behavior
	// Do not apply any changes to the world here
	// HasStateEnded, if false state will update until state ends
	// HasStateEnded, if true state ends
	UFUNCTION(BlueprintImplementableEvent, Category = "Dynamic State Machine")
	void OnBeginStateEvent(bool& HasStateEnded) const;
	virtual void OnBeginState(bool& HasStateEnded) const {};

	// Event fired always after OnBeginStateEvent (Even if HasStateEnded is true)
	// Changes to data-references in OnBeginStateEvent should get applied in this function to the world
	// You can update data-references here which are related to changes in the world (e.g. creating a widget)
	UFUNCTION(BlueprintImplementableEvent, Category = "Dynamic State Machine")
	void ApplyStateBeginEvent();
	virtual void ApplyStateBegin() {};

	// Event fired every tick as long as this node is active
	// Inside this node you should update data-references based on the defined behavior
	// Do not apply any changes to the world here
	// HasStateEnded, if false state will update until state ends
	// HasStateEnded, if true state ends
	UFUNCTION(BlueprintImplementableEvent, Category = "Dynamic State Machine")
	void OnUpdateStateEvent(float DeltaTime, bool& HasStateEnded) const ;
	virtual void OnUpdateState(float DeltaTime, bool& HasStateEnded) const {};

	// Event fired every tick after OnUpdateStateEvent (Even if HasStateEnded is true)
	// Changes to data-references in OnUpdateStateEvent should get applied in this function to the world
	// You can update data-references here which are related to changes in the world (e.g. creating a widget)
	UFUNCTION(BlueprintImplementableEvent, Category = "Dynamic State Machine")
	void ApplyStateUpdateEvent();
	virtual void ApplyStateUpdate() {};

	// Event fired when state ends
	// Last chance here to update data references based on the node behavior
	// Do not apply any changes to the world here
	UFUNCTION(BlueprintImplementableEvent, Category = "Dynamic State Machine")
	void OnEndStateEvent() const;
	virtual void OnEndState() const {};

	// Event fired after OnEndStateEvent
	// Last chance here to apply changes of data references to the world
	// You can update data-references here which are related to changes in the world (e.g. destroying a widget)
	UFUNCTION(BlueprintImplementableEvent, Category = "Dynamic State Machine")
	void ApplyStateEndEvent();
	virtual void ApplyStateEnd() {};

	// With this function you access/update referenced data-assets, data asset must be defined inside _writableDataReferences or _readOnlyDataReferences
	// If requested asset is a _writableDataReferences a reference to the dataAsset is returned
	// If requested asset is a _readOnlyDataReferences a copy of the dataAsset is returned
	// Key must be unique for both writable and read only refs
	// DataAssetType sets the output type of the node, a cast will be performed into this type
	// Failure can be checked with the success node
	UFUNCTION(BlueprintCallable, Category = "Dynamic State Machine", meta = (DeterminesOutputType = "CastToType", DynamicOutputParam = "dataAsset"))
	void GetData(FName key, TSubclassOf<UDSMDataAsset> castToType, UObject*& dataAsset, bool& success) const;
	TWeakObjectPtr<UDSMDataAsset> GetData(FName key) const;

	// Returns the owning DSM Manager, which manages this and all other DSM nodes in the scene
	UFUNCTION(BlueprintCallable, Category = "Dynamic State Machine")
	ADSMGameMode* GetDSMManager() const;

	// Sets the current DSM manager + selfTransition callbacks
	void SetDSMManager(TWeakObjectPtr<class ADSMGameMode> owner, TFunction<bool(TWeakObjectPtr<UDSMDefaultNode>)> selfTransitionCB) 
	{
		_ownerRef = owner;
		_requestSelfTranstion = selfTransitionCB;
	}

	// Returns the DSM save game located in the current DSM manager
	TWeakObjectPtr<class UDSMSaveGame> GetDSMSaveGame() const;
	
	// Checks if a data asset name is referenced by this node
	// Returns nullptr, if data asset is not contained in _writableDataReferences or _readOnlyDataReferences
	// Otherwise reference to the data asset is returned
	TWeakObjectPtr<UDSMDataAsset> ValidateDataAssetByName(const FName& name) const;
	
	// Validates condition groups
	bool ValidateConditionGroups(bool reset = false);
	
	// Runtime evaluation of the validated condition groups 
	// Function must be called before validation
	bool EvaluateConditionGroups() const;

	// Runtime evaluation of all enter conditions, including ConditionGroups, CanEnterStateEvent, and CanEnterState
	bool EvaluateEnterConditions(bool IsSelfTransition, TTuple<FString, FDSMDebugConditions>& debugInfo) const;

	// Check if a name is defined in _ConditionDefinitions
	bool ValidateConditionName(const FName& name) const;
protected:

	// Requests DSM Management System to transition to this node
	// Can be only triggered by the DSM node itself (this node)
	// This makes sure all the behavior regarding this node stays in this node
	// This function allows some flexibility to react properly to events
	// RetValue, self transition is only possible if there is no other active node at the moment
	// RetValue, if false, self transition failed
	UFUNCTION(BlueprintCallable, Category = "Dynamic State Machine", meta = (BlueprintProtected))
	bool RequestDSMSelfTransition();

	// Stores validated expressions, which are ready for evaluation
	UPROPERTY()
	TArray<FExpressionEvaluator> _expressionEvaluators = {};

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

#if WITH_EDITOR
	virtual void PostInitProperties() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
private:
	TFunction<bool(TWeakObjectPtr<UDSMDefaultNode>)> _requestSelfTranstion = nullptr;
	TWeakObjectPtr<class ADSMGameMode> _ownerRef = nullptr;
	bool bCanEnter = true;
};
