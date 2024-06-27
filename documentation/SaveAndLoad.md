# Save and Load 

 We can create custom gameplay using the tools explained in the previous sections. A very important part in games nowadays is to save the game state and laod it at any time. Fortunately, we get this feature for free when using DSM.

 ## Theory

DSM splits logic from data. Logic is contained in ```DSM Nodes``` while data is contained in ```DSM Data Assets```. When a node is activated the referenced ```DSM Data Assets``` can get updated and the changes are applied to the world. So the latest version of all referenced ```DSM Data Assets``` reflect the state of the world. We can use this fact, to create a save game from the referenced ```DSM Data Assets```. 

In order to create a deterministic save game, we must keep track of the order of actions, which were performed in the world. For this reason, DSM stores the history of all activated DSM Nodes, including the changes of the data. This looks like the following : 

<p align="center">
    <img src=images/StateMachineHistory.png width="600"/>
</p>

The system keeps track of all activated ```DSM Nodes```. After a ```DSM Node``` finishes a history element is added to the list. Each history element contains the following information

| Parameter| Description|
| --------| -----------|
| Node Reference| Reference to the ```DSM Node``` object in the world, if still exists |
| Node Label| Unique name of the ```DSM Node```. In case the object is created at runtime, we can use this name to find the object. |
| Node Class | Reference to the class of the ```DSM Node``` |
| Owner | The actor which own the ```DSM Node```|
| Owner Label | Unique name of the owning actor. In case the actor is created at runtime, we can use this name to find the actor. |
| Owner Class | Class type of the owning actor. |
| Data | A copy of all mutated data assets by this node |

It is important to mention that the data stored in a history element is always copied. So each history element only contains the changed ```DSM Data Assets``` by that node. In addition, we store information about the actor which owns the ```DSM Data Asset```. All these information become relevant, when loading the save game. 

In case we want to write the save game to disc, we move all the history elements to a [UPackage](https://docs.unrealengine.com/4.27/en-US/API/Runtime/CoreUObject/UObject/UPackage/) and save it. This has the advantage, that the Unreal Engine will keep track of all the referenced objects in our ```DSM Data Assets```. Additionaly we save the ```DSM History``` to a slot. 

Next time we want to play our game, we need to load the save game. Therefore, we reset the current level to its default and recreate all the actions the player has made in the correct order. Let us have a look at the following pseudo code :

```cpp
void LoadSaveGame()
{
    ResetLevel();

    TArray<SaveGameElement> history = LoadHistoryFromDisc();
    for (auto historyElement : history)
    {
        // We need to find the DSM Node in the world of the history element
        // In case the DSM Node exist in the default level, use the stored reference
        // If the node is not in the default level, we try find it by its unique name
        DSMDefaultNode* referencedNode = GetComponentFromHistoryElement(historyElement);
        if (!referencedNode)
        {
            // We could not find the referenced node, we print an error here
            return;
        }
        // Add data asset changes of the node to the data manager
        PushStateMachineData(history.Data);
        // Apply data asset state to the world
        referencedNode->ApplyStateBegin();
        referencedNode->ApplyStateUpdate();
        referencedNode->ApplyStateEnd();
    }
}
```

First, we load the history from disc. Then we iterate over all history elements to recreate the state, from the original session. In each iteration, we must find the associated ```DSM Node``` in the world. Fortunately, we stored earlier the reference and the unique name of the node and its owner in the history. Based on these information, we try to find the ```DSM Node``` in the world. If this does not succeed for any reason, we throw an error. After we got the ```DSM Node```, we update the data references using the data stored in the history element. The history data contains already all changes made by this specific ```DSM Node```. The only remaining action to do is to apply these state changes to the world by calling the associated ```ApplyState...``` methods. After iterating over all history elements, the old session is recreated. 


## Create a Save Game

You can save the current game state using the ```DSM Game Mode```. The blueprint code shown here is analogue to the usage in C++. Let us have a look at the following example code :

<p align="center">
    <img src=images/SaveGameToSlot.png width="1100"/>
</p>

From the ```DSM Game Mode``` you can get access to the ```StateMachineData``` which contains all the Save/Load functionality. In the example shown here, we use the ```GetRecentHistoryIndexByClass``` function to search the history for the last element from class ```BP_DSM_CreatingSpawnPoint```. We save the current state by passing a ```History Index``` and the ```Slot Name``` which will be the name of our save game. When ticking the flag ```KeepState```, the save game will save the entire history. So on load we recreate the scene until reaching the history index, but we will keep the complete history data. 

There is an alternative version of SaveState which does not block the main thread. The implementation is very similar : 

<p align="center">
    <img src=images/AsyncSaveGameToSlot.png width="1100"/>
</p>

The ```AsyncSaveState``` method works the same way as the blocking alternative, but it is associated with an event ```AsyncSaveFinished```, which is fired when saving has finished. 

A new folder with the name of the save game will popup in your content folder, when saving the current history. This folder contains all the saved ```DSM Data Assets```. Feel free to analyze the ```DSM Data Assets``` associated to the save game.

> **Note**
> Be aware, when saving the ```DSM History``` inside of an active ```DSM Node```, that the history can never contain the current active ```DSM Node```, because the node is added to the history after it ends.

## Load Save Game

A previously stored save game can be loaded by calling the ```LoadState``` function inside the ```StateMachineData```. The shown Blueprint usage is anologue to the C++ version.

<p align="center">
    <img src=images/LoadState.png width="1100"/>
</p>

The ```StateMachineData``` can be accessed from the ```DSMGameMode```. We simply need to call ```LoadState``` using the correct ```SlotName```. When ticking the flag ```DeleteSlotAfterLoad```, we delete the save game from disc after loading is finished. 
```LoadState``` will reset the current level, and calls the ```ApplyState...``` methods of the ```DSM Nodes``` from the history to deterministically recreate the original state.

> **Note**
> In case there is an issue with the save game, you can delete the Saved folder inside the Unreal Project. In addition you need to delete the ```UPackage```, which is created inside the Content folder with the same name as the save game. In case you only delete one of the two folders, the behavior of DSM is undefined. 

## API Information

The ```StateMachineData``` inside the ```DSMGameMode``` provides the following properties and functions :

- **OnAsyncSaveFinished** : Subscribable delegate, which is fired when ```AsyncSaveState``` finishes 
- **Data** : Contains the latest version of all referenced ```DSM Data Assets``` retrieved from the ```DSM History```

| GetRecentHistoryIndexByClass| Iterates backwards over the history and returns the first found node of the passed type |
| --------| -----------|
| **In:** Type| Type of ```DSM Node``` to search in the history |
| **Return** | Index of the last element in the ```DSM History``` with passed Type |


| GetStateMachineHistory | Returns the entire state machine history |
| --------| -----------|


| AsyncSaveState | Saves DSM History to disc in non-blocking way |
| --------| -----------|
| **In:** HistoryIndex | History is stored until the passed history index |
| **In:** SlotName | Name of the Save Game |
| **In:** KeepState| If true, save game stores the entire history. On load, the scene is recreated until reaching the history index, but entire data history is kept. |

| SaveState | Saves DSM History to disc. Blocks main thread. |
| --------| -----------|
| **In:** HistoryIndex | History is stored until the passed history index |
| **In:** SlotName | Name of the Save Game |
| **In:** KeepState| If true, save game stores the entire history. On load, the scene is recreated until reaching the history index, but entire data history is kept. |

| LoadState | Resets the current level and loads the save game |
| --------| -----------|
| **In:** SlotName | Name of the Save Game |
| **In:** DeleteSlotAfterLoad | If true, save game is deleted after finish loading. |

## Conclusion

When using DSM correctly, we get save and laod functionality for free by storing the history of changed ```DSM Data Assets```. DSM provides a simple API for saving and loading a state using slot names. 

[Next page](DebugTools.md)

