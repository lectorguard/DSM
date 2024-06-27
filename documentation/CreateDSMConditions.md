# Create DSM Conditions

## Theory

We now have a basic understanding of how to create a node, add data references, and define its behavior. However, one crucial aspect remains: establishing the rules that determine when our Node becomes active. The DSM framework relies on certain assumptions, which are outlined below:

```
1. All active nodes in the scene are managed by the DSM Game Mode
2. Only one node can be active at the same time
3. Activation of a node is defined by the node itself
```

So, how do we define the condition that must be satisfied for our node to become active? The answer to this question are ```DSM Policies```. You can create custom policies and assign them to some or all ```DSM Nodes```. In this section the focus is on the ```DSM Default Policy```, which is the default policy for every newly created ```DSM Node```. We will later see, how to customize the policies of a ```DSM Node```. In most cases, using the ```DSM Default Policy``` is sufficient. 

## Implement Default Policy Conditions

You can implement entry conditions for a DSM node using ```Blueprint```, ```C++``` or just the ```Details Panel```. Depending on the complexity of your condition you can choose the best way to implement it.

## Implement Conditions using the Details Panel

This way of implementing conditions is especially interesting. Instead of implementing a fixed entry conditions for a ```DSM Node```, we can customize the condition for each ```DSM Node``` instance attached to an actor. So we can reuse the same ```DSM Node``` for multiple actors, but with different entry conditions. Here is an example usage of Details Panel Conditions :

<p align="center">
	<img src=images/Conditions.png />
</p>

You can find the ```Conditions``` information, when selecting the ```DSM Node``` component in the ```Details Panel```. Conditions are devided into ```Condition Definitions``` and ```Condition Groups```. Inside the ```Condition Definitions```, we define conditions and bind them to a name. Inside the ```Condition Groups``` we can create a C++ styled boolean expression using the condition names. If all expressions inside the ```Condition Groups``` evaluate to true, the ```DSM Node``` can become active. You can validate your created expressions directly inside the editor for correctness. When ticking the flag ```Bind Conditions```, an internal check is performed which checks if all the provided field names are correct and if the semantic and syntax of the condition expressions is also correct. In case something is wrong, the ```Bind Conditions``` flag remains false and a warning with the issues is printed to the ```Output Log```. Make sure that ```Bind Conditions``` is set to true for all ```DSM Nodes```, before starting the game. Otherwise a warning will be printed to the Output Log and DSM will not work as intended.

> **Note**
> All the conditions defined inside the ```Condition Groups``` are &&ed during evaluation. So if a single condition fails, the entire node fails. 

Here is a list of all predefined Condition types you can use in the Details Panel :

| Condition Type | Description|
| --------| -----------|
| DSMConditionTrue | Condition returning always true. Mainly used for debugging. |   
| DSMConditionFalse | Condition returning always false. Mainly used for debugging. |
| DSMConditionBool | Binds a boolean from a ```DSM Data Asset``` defined inside ```WritableDataReferences``` or  ```readOnlyDataReferences```. Returns the current value of this boolean.|
| DSMConditionComponentOverlap | Binds an overlappable component of the ```DSM Node``` owning actor. Examples for overlappable components are ```BoxComponent``` or ```SphereComponent```. Returns true, if player overlaps the bound component.  |
| DSMConditionPointerValid | Binds an object pointer from a ```DSM Data Asset``` defined inside ```WritableDataReferences``` or  ```readOnlyDataReferences```. Returns true if the pointer is valid. (Not nullptr, or pending kill or similar) |
| DSMConditionContainedInArray | Checks if a property is contained inside a TArray. Currently supported property types are ```FName``` and ```UClass```. The property as well as the array must be defined in a ```DSM Data Asset```, referenced inside ```WritableDataReferences``` or  ```readOnlyDataReferences```. Returns true, if property is inside the array. |
| DSMConditionCompare | Binds two properties from ```WritableDataReferences``` or  ```readOnlyDataReferences``` and compares them. Returns true if properties are equal. |
| DSMConditionNumberTypeCompare | Binds two number typed properties from ```WritableDataReferences``` or  ```readOnlyDataReferences``` and compares them based on a comparison operator. Currently supported types are ```int32``` and ```float```. Returns true if comparison expression evaluates to true. |


## Create Custom Condition Type for the Details Panel

You can create custom ```DSM Condition Types``` for the details panel using C++. You simply need to create a new C++ class, which inherits from ```UDSMConditionBase```. In the following, we show the implemenation of ```DSMConditionTrue``` : 

```cpp
#include "DSMCondition.h"

// Blueprintable allows the condition to be used with blueprints
// DefaultToInstanced and editinlinenew allows us to create instances of this object 
// inside the editor, outside of the runtime
UCLASS(Blueprintable, DefaultToInstanced, editinlinenew)
class DYNAMICSTATEMACHINE_API UDSMConditionTrue : public UDSMConditionBase
{
	GENERATED_BODY()

public:
	// This method is called at runtime and returns the result of the condition, 
	// given the current state 
	bool Evaluate(TWeakObjectPtr<const UDSMDefaultNode> defaultNode) const override 
	{ 
		return true; 
	};
	// This method is called inside the editor to validate the defined condition
	// It should return true, if all used properties can be validated
	bool BindCondition(TWeakObjectPtr<UDSMDefaultNode> defaultNode) const override
	{
		return true; 
	};
};

```

This is a very simple implementation, for more complex examples please have a look at the header file ```DynamicStateMachine\Source\DynamicStateMachine\Classes\DSMCondition.h```. Basically, you only need to override the two given virtual functions. When overriding the virtual functions, be aware that code inside the ```Evaluate``` function runs at runtime and the code inside ```BindCondition``` runs in the editor. Code in the editor can behave differently to code at runtime. Please checkout some sample implementation to see the differences.

## Implement Conditions inside Blueprints

You can also implement your conditions inside blueprints :

<p align="center">
	<img src=images/BlueprintConditions.png />
</p>

Inside your ```DSM Node```, you can simply override the ```CanEnterStateMethod``` and define custom conditions. The function takes a boolean called ```IsSelfTransitioned```, which is true, if the transition was caused by the node itself. We will see later how we can do that. In this example, our condition is a simple check for the ```IsSelfTransitioned```. The ```CanEnterStateMethod``` returns a map with key and value types ```Name``` and ```Boolean```. Similar to the details panel version, we must give each condition a description btw. a name. Later the names will be useful to debug our nodes. 

> **Note**
> All the conditions defined in the output map will be &&ed during evaluation. So if a single condition fails, the entire node fails. 

## Implement Conditions in C++

When creating your custom ```DSM Node``` in C++, you can simply override the following virtual method :

```virtual void CanEnterState(bool bIsSelfTransitioned, TMap<FName, bool>& CanEnter) const;```

The function works the same way as the blueprint alternative. It is recommended to use the blueprint and the details panel version, because they are easier to use. 

> **Note**
> The usage of any shown condition implementations is optional. So you can freely decide to use the blueprint condition and/or details panel conditions and/or c++ conditions.

## Conclusion

In this section we learned how to use conditions using the ```DSM Default Policy```. Conditions can be implemented in C++, Blueprints and the Detail Panel. 

[Next page](CreateDSMTransitions.md)


