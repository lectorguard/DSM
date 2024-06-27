// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "DSMDefaultNode.h"
#include "Misc/Optional.h"
#include "DSMLogInclude.h"
#include "Engine/DataAsset.h"
#include "Algo/Reverse.h"
#include "Serialization/ObjectReader.h"
#include "Serialization/ObjectWriter.h"
#include "QuestConversationDataAsset.h"
#include "DSMDataAsset.h"
#include "UObject/Package.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "HAL/PlatformFilemanager.h"
#include "DSMSaveGame.generated.h"




/**
 * Save Game representation of a node for the DSM State Machine History
 * Stores the node, its owner, and the referenced data assets
 * Each NodeID stores an own copy of the referenced data assets
 */
USTRUCT(BlueprintType)
struct DYNAMICSTATEMACHINE_API FDSMNodeID
{
	GENERATED_BODY()
	
	// Node object ptr
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Node")
	TWeakObjectPtr<UDSMDefaultNode> _node = nullptr;

	// Name of the node object
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Node")
	FName _nodeLabel = NAME_None;

	// Class of the node
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Node")
	TSubclassOf<UActorComponent> _nodeClass = nullptr;

	// Node owning actor ptr
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Owner")
	TWeakObjectPtr<AActor> _owner = nullptr;

	// Node owning actor name
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Owner")
	FName _ownerLabel = NAME_None;

	// Node owning actor class
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Owner")
	TSubclassOf<AActor> _ownerClass;

	// Referenced/modified data assets of the node
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Data")
	TMap<FName, TObjectPtr<UDSMDataAsset>> _data;

	// Names of the referenced/modified data assets
	UPROPERTY()
	TMap<FName, FName> _dataRaw;

	// We move the associated data assets to the new package
	// The package will get saved later on
	void PrepareSerialization(TObjectPtr<UPackage> newPackage)
	{
		_dataRaw.Empty(_data.Num());
		for (const TTuple<FName, TObjectPtr<UDSMDataAsset>>& elem : _data)
		{
			// Add values to the new package we are going to save
			elem.Value->Rename(nullptr, newPackage);
			// Store the raw names of the data assets, that we can find them again on load
			_dataRaw.Add(elem.Key, elem.Value->GetFName());
		}
	}

	// We look for the previously stored data-assets and recreate the data-references
	void PostDeserialization(const TMap<FName, UObject*>& objectsInPackage)
	{
		_data.Empty(_dataRaw.Num());
		for (const TTuple<FName, FName>& elem : _dataRaw)
		{
			if (objectsInPackage.Contains(elem.Value))
			{
				_data.Add(elem.Key, Cast<UDSMDataAsset>(objectsInPackage[elem.Value]));
			}
			else
			{
				UE_LOG(LogDSM, Error, TEXT("Could not find data asset with name %s"), *elem.Value.ToString());
			}
		}
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAsyncSaveFinished);

/**
 * Holds the entire DSM state machine history and information relevant for the save game
 * Contains save and load functionality
 */
UCLASS(DefaultToInstanced, BlueprintType)
class DYNAMICSTATEMACHINE_API UDSMSaveGame : public USaveGame
{
	GENERATED_BODY()

	// Debug package name, for debuggin purposes only
	const FString _debugSaveName = "DSMDebug";

	// Referenced data asset in history will be stored in this package
	UPROPERTY()
	TObjectPtr<UPackage> _dsmPackage = nullptr;
protected:
	// Stores history of executed node
	// Each node stores a copy of the referenced data, when the node finished executing
	UPROPERTY(VisibleAnywhere, Category = "DSM History")
	TArray<FDSMNodeID> _stateMachineHistory{};

public:

	// Callback after async saving has finished
	UPROPERTY(BlueprintAssignable, Category = "DSM Save Game")
	FOnAsyncSaveFinished OnAsyncSaveFinished;

	// Should state be kept when loading a history element
	UPROPERTY(EditAnywhere, Category = "DSM History")
	bool _keepState = false;

	// Loads state machine history until index, when hitting enter 
	UPROPERTY(EditAnywhere, Category = "DSM History")
	int32 _indexToLoad = -1;

	// Iterates backwards over the history
	// Returns the index of the first element which has certain type
	// E.g. Can be used to find the index of the last save point, or similar
	UFUNCTION(BlueprintCallable, Category = "DSM History")
	int32 GetRecentHistoryIndexByClass(TSubclassOf<class UDSMDefaultNode> type) const;

	// Returns the entire state machine history
	UFUNCTION(BlueprintCallable, Category = "DSM History")
	TArray<FDSMNodeID> GetStateMachineHistory() const { return _stateMachineHistory; }

	// Async saves the DSM history to disc using the defined slot name
	// You can subscribe the OnAsyncSaveFinished delegate to get a callback, when saving has finished
	// In order to delete the save game from disc, you need to delete the save game folder and the save game package (both have same name)
	// You can find the save game package inside the content folder or the save game folder
	UFUNCTION(BlueprintCallable, Category = "DSM Save Game")
	void AsyncSaveState(int32 historyIndex, const FString& slotName, bool keepState = false);

	// Saves the DSM history to disc using the defined slot name
	// In order to delete the save game from disc, you need to delete the save game folder and the save game package (both have same name)
	// You can find the save game package inside the content folder or the save game folder
	UFUNCTION(BlueprintCallable, Category = "DSM Save Game")
	void SaveState(int32 historyIndex, const FString& slotName, bool keepState = false) const;

	// Loads previously stored state, either from disc or from memory
	// deleteSlotAfterLoad, if true save game and package is deleted after load
	UFUNCTION(BlueprintCallable, Category = "DSM Save Game")
	void LoadState(const FString& slotName, bool deleteSlotAfterLoad) const;

	// Replaces the state machine history
	// This is called on load
	void SetStateMachineHistory(const TArray<FDSMNodeID>& history)
	{
		_stateMachineHistory = history;
		UpdateData();
	}

	// Adds an element to the state machine history
	void PushStateMachineElement(const FDSMNodeID& node)
	{
		_stateMachineHistory.Push(node);
		UpdateData();
	}

	// Creates a package and a save game files
	// Stores DSM history to save game and referenced data assets to package
	void PrepareSerialization(const FString& slotName);

	// Loads package and save game from disc or from memory
	// Moves referenced data assets to transient package and recreates the previous history
	void PostDeserialization(TWeakObjectPtr<UObject> parent, const FString& slotName);

	// Adds a node element to the history
	void AddMemory(TWeakObjectPtr<UDSMDefaultNode> node, const TMap<FName, TObjectPtr<UDSMDataAsset>>& DataReferences);


	// Returns a deep copy of the current version of a data asset if it exists. Otherwise the default data-asset is returned
	TObjectPtr<UDSMDataAsset> GetDataCopy(const TWeakObjectPtr<UDSMDataAsset> DefaultDataAssetObject) const;

private:
	
	// Searches for the latest version of a data asset inside the history
	TWeakObjectPtr<UDSMDataAsset> GetLatestDataAssetOfType(const TWeakObjectPtr<UDSMDataAsset> DefaultDataAssetObject) const;

private:

	// Shows the latest version of all referenced data assets in the editor for debug purposes
	void UpdateData();

	// Converts a save game name to a package name path
	FString NameToPackageName(const FString& name){	return FString::Printf(TEXT("/Game/%s/%s"), *name, *name);}

	// Contains the latest version of all referenced data assets retrieved from the history
	UPROPERTY(EditAnywhere, Category = "DSM State")
	TMap<FName, TWeakObjectPtr<UDSMDataAsset>> _data;

#if WITH_EDITOR
	bool CanEditChange(const FProperty* InProperty) const override;
#endif

	TObjectPtr<UDSMSaveGame> SaveState_Internal(int32 historyIndex, const FString& slotName, bool keepState = false) const;



public:
	void SaveGame(FString SlotName, TFunction<void(const FString&, int32&, bool&)> OnFinishedSaving);
	static void LoadGame(FString SlotName, TFunction<void(UDSMSaveGame*)> OnLoaded);
};
