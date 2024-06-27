# Debug Tools

In the previous sections, the main DSM functionality for creating ```DSM gameplay``` is described. ```DSM``` supports ```rapid gameplay prototyping``` by providing substential debug functionality. Most of the debug features, show the current state of the system and the decisioning process which led to this state. DSM also supports to change the current game state at runtime and to rollback the game state history in a customizable way.

# DSM Node History

As described in previous sections, DSM stores the entire state history, including the changes to the referenced ```DSM Data```. We can access these history information at runtime inside the editor. 


<p align="center">
    <img src=images/AccessDSMHistory.png width="600"/>
</p>

- Start your example scene in the editor
- Inside the game press ```F8``` to switch from game to editor mode
- Inside the ```World Outliner``` search for your ```DSM Game Mode``` and select it
- You can find the ```DSM History``` inside the ```Details Panel```

The information provided inside the ```DSM History``` is described in the previous [Save and Load](SaveAndLoad.md) section. It is possible to change the provided history information, but it is not recommended. Loading a mutated DSM history is undefined behavior. It is recommended to only change the current state data of the game. But it is possible to rollback the game state to some history index, so you can afterwards update the current state data. DSM provides the following options to rollback the current game state :

| Option  | Description|
| --------| -----------|
| KeepState | If ticked, the current state machine history is kept after rollback. You basically load the history until a certain index, but you keep the old state machine history. This can lead to some logical inconsistencies, because some state information were not applied to the world. If unticked (default), the current state machine history is discarded on load. After load, the state machine history will reflect the laoded index. |
| Index to Load | Enter the history index you want to load. Be aware, when hitting enter the ```DSM History Index``` will be loaded instantly. |

# DSM current state data

The current state data contains the latest version of all writable data assets used in the current game session. You can find the current state data inside the ```Details Panel``` of the current ```DSM Game Mode```. 

<p align="center">
    <img src=images/DSMState.png width="900"/>
</p>

The current state information are located inside the ```Data``` map in the ```Details Panel```. The provided map is updated each time when an active ```DSM Node``` ends. The provided ```DSM Data Assets``` inside ```Data``` is fully customizable at runtime. This can be useful for debugging purposes. For example, a developer could add some items to the ```Player Inventory``` data asset at runtime to check, if a quest finishes correctly.  

# DSM Transition Debug Data

In some situations in your game a DSM node might not work as intended. In these situations, the ```DSM Transition Debug Data``` is helpful. It basically describes the DSM evaluation process for activating a next node or no further node. You can find these information inside the ```Details Panel``` of the ```DSM Game Mode``` under ```State Machine Debug Data```.


<p align="center">
    <img src=images/DSMDebugData.png width="1300"/>
</p>

For each transition in the game, the ```State Machine Debug Data``` creates an entry in its list. Each entry contains a list of  ```Sucessful``` and ```Unsuccessful``` DSM nodes and the ```Elapsed Time``` (Seconds elapsed in the game) of the transition. If the delta between two timestamps is almost zero, it is very likely that the transitions were executed in the same chain. A chain means here the sequence of multiple activated nodes until the ```DSM Game Mode``` goes back to ```Idle``` mode. The ```Sucessful``` DSM node list shows all the nodes, which fullfill all conditions. In the example shown here only the ```BP_DSM_OpenDoor``` node satisfies all of its conditions (```Is door already open, Overlaps Door Collider, Has player collected key```). In the ```Unsuccessful``` DSM node list, we find all the available nodes in the game, which do not satisfy their conditions. For example, the ```BP_DSM_CowMission``` satisfies all its conditions besides the ```Player is inside Trigger``` condition. Also all other nodes inside the list have at least one condition which is not satisfied. The name of the nodes inside the ```Successful``` and ```Unsuccessful``` list are given in the format ```<ActorNameInWorld> -> <DSMNodeName>```.


You can use these information to understand why DSM behaved in a certain way. This makes it easier to identify an issue and to find a solution to it. For example, if a transitions leads to two successful nodes which only support the ```DSM Default Policy```, the ```DSM Game Mode``` will transition to Idle, because no unique successful node could be found. We can easily identify this problem using the ```State Machine Debug Data```. We can fix the issue by either adding an additional condition to one of the nodes or by adding a new policy to both ```DSM Nodes``` which resolve the particular conflict. In case a ```DSM Node``` is not showing up/activated, you can check out the ```Default Nodes``` list, which shows all the currently available nodes in the game. If the ```DSM Node``` you are looking for is missing, something must have gone wrong during the registration process of the node. Please check in this case the ```Output Log``` for additional information.


# Output Log

DSM provides a custom output log category called ```LogDSM```. In case DSM does not work as expected, you can open the Output Log and search for ```LogDSM```. Check if there are any warnings or errors, which are associated to your issue. After fixing the shown issues, DSM should work as intended. 

```LogDSM``` also provides node and policy activation information. After every transition, the ```DSM Game Mode``` logs the activated policy and the associated node to the output log. Please check these information, if a policy is activated unexpectedly.

# Conclusion

DSM is a powerful tool, which allows a developer to create fully customizable (non-linear) gameplay with substential debugging features and save/load functionality for free. 

[Back to Main Page](../README.md)





