# Dynamic State Machine (DSM)


![](documentation/images/DSMPreview.png)

The Dynamic State Machine (DSM) plugin for Unreal Engine 5 provides a deterministic data driven system to create any kind of  (non-linear) gameplay. DSM supports out of the box loading and saving any gameplay state at any time. It is fully integrated in the Unreal Blueprint system and provides a variety of debug features. Check out the provided examples and get convinced of the system now ! 

## Features

- Abstraction layer for (non-linear) gameplay
- Fully data driven
- Immutable data history
- Deterministic
- Seperation between logic and data
- Automatic gameplay transitions based on Node conditions
- Save/Load any gameplay at any time
- Various Debug Feaures
- Documentation
- Backend fully written in C++
- Gameplay creation in Blueprints fully suported
- Highly extendible and customizable
- Example scenes

## Prerequisites

- Unreal Engine 5.1 or higher

## Getting Started

You can either integrate the packaged plugin into your existing project or you can simply open the project inside the code folder. Simply navigate to the Dynamic State Machine plugin and open one of the levels in the example scenes. You can simply press play and do not need to change any setting.

### Integrate Packaged Plugin into Existing Project

You can integrate the packed plugin version of DSM by following these steps:

- Unzip the DSM package inside the package folder
- Copy the Plugin to the ```Plugins``` folder of your custom Project 
- Open your project and navigate to the ```Dynamic State Machine``` folder
- Checkout the provided examples

> **Note**
> If there is no plugins folder in your project simply create a ```Plugins``` folder at the level of the ```.uproject``` file

## Documentation

The following documentation explains how to create custom gameplay using DSM based on an empty scene. 
For each step some background information about the system is provided.

1. [Set Up DSM Level](documentation/SetUpDSMLevel.md)
    1. [Create DSM Data Asset](documentation/CreateDSMDataAsset.md)
    2. [Create DSM Node](documentation/CreateDSMNode.md)
    3. [Create DSM Condition](documentation/CreateDSMConditions.md)
    4. [Create DSM Transition](documentation/CreateDSMTransitions.md)
2. [Save and Load](documentation/SaveAndLoad.md)
3. [Debug Tools](documentation/DebugTools.md)



