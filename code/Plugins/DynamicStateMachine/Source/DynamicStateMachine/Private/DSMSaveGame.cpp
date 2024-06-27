// Fill out your copyright notice in the Description page of Project Settings.


#include "DSMSaveGame.h"
#include <Kismet/GameplayStatics.h>
#include "DSMLogInclude.h"
#include "Kismet/GameplayStatics.h"
#include "DSMManager.h"



int32 UDSMSaveGame::GetRecentHistoryIndexByClass(TSubclassOf<class UDSMDefaultNode> type) const
{
	const int32 index = _stateMachineHistory.FindLastByPredicate([type](const FDSMNodeID& node)
		{
			return node._nodeClass == type;
		});
	return index;
}

void UDSMSaveGame::LoadState(const FString& slotName, bool deleteSlotAfterLoad) const
{
	TWeakObjectPtr<ADSMGameMode> gameMode = Cast<ADSMGameMode>(GetOuter());
	if (gameMode.IsValid())
	{
		ADSMGameMode::SetSaveLoadInfo({ slotName, deleteSlotAfterLoad });
		UGameplayStatics::OpenLevel(GetWorld(), *GetWorld()->GetName(), false);
	}
	else
	{
		UE_LOG(LogDSM, Warning, TEXT("SaveGame owner must be DSMGameMode"));
	}
}



void UDSMSaveGame::PrepareSerialization(const FString& slotName)
{
	const FString packageName = NameToPackageName(slotName);
	// Creates or finds package
	_dsmPackage = CreatePackage(*packageName);

	if (!_dsmPackage)
	{
		UE_LOG(LogDSM, Error, TEXT("Node %s did not read/modify referenced data asset %s and no other node created a data asset instance of this type."));
		return;
	}

	for (FDSMNodeID& node : _stateMachineHistory)
	{
		node.PrepareSerialization(_dsmPackage);
	}

	// Delete old package file
	FString packageFilePath = FPackageName::LongPackageNameToFilename(packageName, FPackageName::GetAssetPackageExtension());
	if (FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*packageFilePath))
	{
		UE_LOG(LogDSM, Log, TEXT("Successfully deleted old DSM package"));

	}
	else
	{
		UE_LOG(LogDSM, Log, TEXT("Failed to delete DSM package"));
	}
	// Dont know why, but if this is called and not even used before saving the package
	// there is no crash
	UMetaData* MetaData = _dsmPackage->GetMetaData();

	FSavePackageArgs args;
	args.TopLevelFlags = EObjectFlags::RF_Public | EObjectFlags::RF_Standalone;
	if (UPackage::SavePackage(_dsmPackage, nullptr, *packageFilePath, args))
	{
		UE_LOG(LogDSM, Log, TEXT("DSM package saved successfully"));
	}
	else
	{
		UE_LOG(LogDSM, Error, TEXT("DSM package save failed"));
	}
}

void UDSMSaveGame::PostDeserialization(TWeakObjectPtr<UObject> parent, const FString& slotName)
{
	const FString packageName = NameToPackageName(slotName);
	// Expect existing package on load
	UPackage* dsmPackage = LoadPackage(nullptr, *packageName, LOAD_NoRedirects);
	if (!dsmPackage)
	{
		UE_LOG(LogDSM, Log, TEXT("Can not find package"));
		return;
	}
	// Get all objects in the packagel
	TArray<UObject*> foundObjects;
	GetObjectsWithPackage(dsmPackage, foundObjects, true);
	TMap<FName, UObject*> foundObjectMap;
	foundObjectMap.Reserve(foundObjects.Num());
	for (UObject* elem : foundObjects)
	{
		// Reparent objects from package to the current world
		TObjectPtr<UObject> copy = DuplicateObject<UObject>(elem, GetTransientPackage());
		foundObjectMap.Add(elem->GetFName(), copy);
		// Delete package object
		elem->RemoveFromRoot();
		elem->ClearFlags(RF_Public | RF_Standalone);
		elem->MarkAsGarbage();
	}
	UE_LOG(LogDSM, Log, TEXT("Found elements in package on deserialization %d"), foundObjects.Num());
	for (FDSMNodeID& node : _stateMachineHistory)
	{
		node.PostDeserialization(foundObjectMap);
	}
}

void UDSMSaveGame::AddMemory(TWeakObjectPtr<UDSMDefaultNode> node, const TMap<FName, TObjectPtr<UDSMDataAsset>>& DataReferences)
{
	TMap<FName, TObjectPtr<UDSMDataAsset>> copiedInstances = DataReferences;
	for (TTuple<FName, UDSMDataAsset*> tuple : DataReferences)
	{
		if (tuple.Value)
		{
			copiedInstances[tuple.Key] = tuple.Value;
		}
		else
		{
			UE_LOG(LogDSM, Log, TEXT("Node %s did not read/modify referenced data asset %s and no other node created a data asset instance of this type."),
				*node->GetOwner()->GetName(), *(tuple.Value ? tuple.Value->GetName() : FString("None")));
			UE_LOG(LogDSM, Log, TEXT("Data asset will not be stored in history, if history is loaded and the data asset of the type above is requested, the Default Data Asset will be used"));
		}
	}

	FDSMNodeID newNode;
	// If node or actor was destroyed during node, we still try to get the instance for the history
	if (node.IsValid(true))
	{
		newNode._node = node;
		newNode._nodeLabel = node.Get(true)->GetFName();
		newNode._nodeClass = node.Get(true)->GetClass();
		newNode._owner = node.Get(true)->GetOwner();
		newNode._ownerClass = node.Get(true)->GetOwner()->GetClass();
		newNode._ownerLabel = node.Get(true)->GetOwner()->GetFName();
	}
	newNode._data = copiedInstances;
	_stateMachineHistory.Emplace(newNode);
	UpdateData();
}

TObjectPtr<UDSMDataAsset> UDSMSaveGame::GetDataCopy(const TWeakObjectPtr<UDSMDataAsset> DefaultDataAssetObject) const
{
	TWeakObjectPtr<UDSMDataAsset> latestVersion = GetLatestDataAssetOfType(DefaultDataAssetObject);
	TObjectPtr<UDSMDataAsset> copy = nullptr;
	if (latestVersion.IsValid())
	{
		copy = DuplicateObject<UDSMDataAsset>(latestVersion.Get(), GetTransientPackage());
	}
	else
	{
		copy = DuplicateObject<UDSMDataAsset>(DefaultDataAssetObject.Get(), GetTransientPackage());
	}
	if (copy)
	{
		copy->OnRequestDeepCopy(copy);
	}
	return copy;
}

TWeakObjectPtr<UDSMDataAsset> UDSMSaveGame::GetLatestDataAssetOfType(const TWeakObjectPtr<UDSMDataAsset> DefaultDataAssetObject) const
{
	if (DefaultDataAssetObject.IsValid())
	{
		TArray<FDSMNodeID> historyCopy = _stateMachineHistory;
		Algo::Reverse(historyCopy);
		for (const FDSMNodeID& node : historyCopy)
		{
			if (node._data.Contains(DefaultDataAssetObject->GetFName()))
			{
				return node._data[DefaultDataAssetObject->GetFName()];
			}
		}
	}
	else
	{
		UE_LOG(LogDSM, Warning, TEXT("Passed data asset is invalid !!"));
	}
	return nullptr;
}

void UDSMSaveGame::UpdateData()
{
	_data.Empty();
	TArray<FDSMNodeID> historyCopy = _stateMachineHistory;
	Algo::Reverse(historyCopy);
	for (const FDSMNodeID& node : historyCopy)
	{
		for (const TTuple<FName, TObjectPtr<UDSMDataAsset>>& elem : node._data)
		{
			if (!_data.Contains(elem.Key))
			{
				_data.Add(elem);
			}
		}
	}
}

void UDSMSaveGame::AsyncSaveState(int32 historyIndex, const FString& slotName, bool keepState /*= false*/)
{
	
	FAsyncSaveGameToSlotDelegate SavedDelegate;
	// USomeUObjectClass::SaveGameDelegateFunction is a void function that takes the following parameters: const FString& SlotName, const int32 UserIndex, bool bSuccess
	SavedDelegate.BindLambda([this](const FString& s, const int32 i , bool b)
		{
			OnAsyncSaveFinished.Broadcast();
		});

	// Start async save process.
	TObjectPtr<UDSMSaveGame> saveGame = SaveState_Internal(historyIndex, slotName, keepState);
	UGameplayStatics::AsyncSaveGameToSlot(saveGame, slotName, 0, SavedDelegate);
}

void UDSMSaveGame::SaveState(int32 historyIndex, const FString& slotName, bool keepState /*= false*/) const
{
	TObjectPtr<UDSMSaveGame> saveGame = SaveState_Internal(historyIndex, slotName, keepState);
	UGameplayStatics::SaveGameToSlot(saveGame, slotName, 0);
}


TObjectPtr<UDSMSaveGame> UDSMSaveGame::SaveState_Internal(int32 historyIndex, const FString& slotName, bool keepState /*= false*/) const
{
	if (historyIndex < 0 || historyIndex >= _stateMachineHistory.Num())
	{
		UE_LOG(LogDSM, Warning, TEXT("Invalid Index passed to save/load state."));
		return nullptr;
	}

	TArray<FDSMNodeID> relevantNodes = {};
	if (keepState)
	{
		relevantNodes = _stateMachineHistory;
	}
	else
	{
		relevantNodes.Append(_stateMachineHistory.GetData(), historyIndex + 1);
	}
	TWeakObjectPtr<ADSMGameMode> gameMode = Cast<ADSMGameMode>(GetOuter());
	if (gameMode.IsValid())
	{

		TObjectPtr<UDSMSaveGame> saveGame = NewObject<UDSMSaveGame>();
		saveGame->_stateMachineHistory = relevantNodes;
		saveGame->_indexToLoad = historyIndex;
		saveGame->_keepState = keepState;
		saveGame->PrepareSerialization(slotName);
		return saveGame;
		
	}
	else
	{
		UE_LOG(LogDSM, Warning, TEXT("SaveGame owner must be DSMGameMode"));
	}
	return nullptr;
}

#if WITH_EDITOR
bool UDSMSaveGame::CanEditChange(const FProperty* InProperty) const
{
	const bool ParentVal = Super::CanEditChange(InProperty);

	// Can we edit flower color?
	if (InProperty->GetFName() == GET_MEMBER_NAME_CHECKED(UDSMSaveGame, _indexToLoad))
	{
		if (_indexToLoad >= 0 && _indexToLoad < _stateMachineHistory.Num())
		{
			SaveState(_indexToLoad, "DSMDefault", _keepState);
			LoadState("DSMDefault", true);
		}
	}

	return ParentVal;
}
#endif


void UDSMSaveGame::SaveGame(FString SlotName, TFunction<void(const FString&, int32&, bool&)> OnFinishedSaving)
{
	// Set up the (optional) delegate.
	FAsyncSaveGameToSlotDelegate SavedDelegate;
	// USomeUObjectClass::SaveGameDelegateFunction is a void function that takes the following parameters: const FString& SlotName, const int32 UserIndex, bool bSuccess
	SavedDelegate.BindLambda(OnFinishedSaving);

	// Start async save process.
	UGameplayStatics::AsyncSaveGameToSlot(this, SlotName, 0, SavedDelegate);
}

void UDSMSaveGame::LoadGame(FString SlotName, TFunction<void(UDSMSaveGame*)> OnLoaded)
{
	FAsyncLoadGameFromSlotDelegate LoadedDelegate;
	LoadedDelegate.BindLambda([OnLoaded](const FString& slotName, int32& userIndex, USaveGame*& loadedGameData)
		{
			if (UDSMSaveGame* saveState = Cast<UDSMSaveGame>(loadedGameData))
			{
				UE_LOG(LogDSM, Log, TEXT("Loading save game slot %s was successful"), *slotName);
				OnLoaded(saveState);
			}
		});
	UGameplayStatics::AsyncLoadGameFromSlot(SlotName, 0, LoadedDelegate);
}
