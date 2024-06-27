# Create DSM Data Asset

Before creating our first ```DSM Node```, we should consider the desired structure of our data necessary for the DSM node.
In the following, the data asset functionality is explained on a simple inventory example. Our inventory and all other data which work with DSM are ```DSM DataAssets```. Data Assets in Unreal Engine 5 are objects, which only contain data.

## Create Data Asset in C++

It is recommended to create Data Assets in C++, because it is easier and less error prone compared to the Blueprint alternative.

```cpp
// You need this include for inheriting from the DSMDataAsset
#include "DSMDataAsset.h"

UCLASS()
class YourProject_API UPlayerInventory : public UDSMDataAsset
{
	GENERATED_BODY()

public:
    // Add all variables for the state in here and expose them to the editor using UPROPERTY
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<TSubclassOf<AActor>> Owning = {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FName> CompletedQuestIDs{};
};
```

Inside your ```DSM Data Assets``` you can define any fields necessary to describe your custom state.


> **Note**
> In case you want to reference other ```DSM Data Assets``` inside your ```DSM Data Asset```, please implement the virtual method ```void OnRequestDeepCopy(UObject* owner)``` and duplicate the referenced ```DSM Data Assets``` using the ```owner``` object passed by the function. This will make sure, that all ```DSM Data Assets``` are added correctly to the history. 

## Create Data Asset in Blueprints

- Right click in the ```Content Browser``` and select ```Blueprint Class```
- Search for class ```DSMDataAsset``` and create the blueprint
- This will be the template data asset from which we will create later instances from
- Add any variables you need for your gameplay logic to it inside the Blueprint Editor
- That's it

> **Note**
> We distinguish later between ```Read Only``` and ```Writable``` data assets, so it can make sense to split your data assets into a read only part and a writable part.
> The writable part will be saved later on. Minimizing the content inside ```Writable Data Assets```, minimizes the file size of our save game.

## Create Data Asset Instances

Now that we have created a template for data assets, we can create some instances which will be used in our gameplay logic.

- Inside the editor make a right click in the content browser and select the ```Miscellaneous``` category
- In this category select ```Data Asset```
- Enter the name of the Data Asset template we just created and create an instance of it
- Give it a **unique name**, DSM does  **not** support data asset instances with the same name
- Thats it  

<p align="center">
	<img src=images/CreateDataAsset.png width="400"/>
</p>

> **Note**
> Of course you can create multiple instances of a template data asset as long as the instance name is unique

## Conclusion

Data assets are the only ressource to communicate between nodes, so you can have a lot of them. Make sure to organize them in a consistent way.

[Next page](CreateDSMNode.md)