# Create DSM Level

Simply open an existing level or create a new one inside the Editor

## Update Game Mode

The DSM system requires a game mode which inherits from ```DSM Game Mode```, which is shipped with the plugin. The ```DSM Game Mode``` manages the state machine. It holds references to all nodes and schedules node transitions. When your game starts, each DSM node registers at the ```DSM Game Mode``` instance in the world.

## Create DSM Game Mode

- Right click in the content browser and select Blueprint class
- Type in ```DSMGameMode``` and create an instance
- Open the ```World Settings``` inside your level
- Search for the GameMode override option and select you newly created ```DSMGameMode```

<p align="center">
    <img src=images/SetGameModeLevel.png width="600"/>
</p>

> **Note**
> You can set a default game mode in the project settings, so newly created levels always use your custom ```DSMGameMode```

> **Note**
> If you have already have a custom game mode (Not using DSM), you can open the class settings in the blueprint editor and set Parent Class to ```DSMGameMode```
> In case you have a custom C++ game mode, you can simply switch the inheritance from ```GameModeBase``` to ```DSMGameMode```

## DSM Game Mode Settings

Most of the ```DSM Game Mode``` specific settings are for debugging purposes, which will be discussed later.

| Setting | Description|
| --------| -----------|
| Request Transition After Begin Play | If ticked, a transition is triggered after node registration process has finished on play

## Conclusion

```DSM Game Mode``` is the heart of the DSM system and must be present in every level which uses any DSM features.

[Next page](CreateDSMDataAsset.md)
