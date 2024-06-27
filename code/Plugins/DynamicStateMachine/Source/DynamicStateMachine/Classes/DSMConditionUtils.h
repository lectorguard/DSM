// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/DataAsset.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "DSMLogInclude.h"
#include "DSMConditionUtils.generated.h"

/**
 * Contains executable function to evaluate a compiled C++ bool expression
 */
USTRUCT()
struct DYNAMICSTATEMACHINE_API FExpressionEvaluator
{
	GENERATED_BODY()

	using EvalCB = TFunction<bool(TWeakObjectPtr<const class UDSMDefaultNode>)>;

	FExpressionEvaluator() {};
	
	// Constructor for ! operators
	FExpressionEvaluator(const EvalCB& toNegate)
	{
		check(toNegate);
		_evaluateFunction = [toNegate](TWeakObjectPtr<const class UDSMDefaultNode> ptr)
		{
			return !toNegate(ptr);
		};
	}

	// Constructor for AND and OR operations
	FExpressionEvaluator(const EvalCB& left, const EvalCB& right, bool bIsAnd)
	{
		check(left);
		check(right);
		_evaluateFunction = [left, right, bIsAnd](TWeakObjectPtr<const class UDSMDefaultNode> ptr)
		{
			const bool bleft = left(ptr);
			const bool bright = right(ptr);

			// Uncomment this to print the final evaluation order of the tokens
			//UE_LOG(LogDSM, Warning, TEXT("Evaluating %s %s %s "), *(bleft ? FString("true") : FString("false")),
			//	*(bIsAnd ? FString("&&") : FString("||")), *(bright ? FString("true") : FString("false")));
			if (bIsAnd)
			{
				return bleft && bright;
			}
			else
			{
				return bleft || bright;
			}
		};
	}

	// Returns a function ptr, which evaluates the expression
	EvalCB GetEvaluateFunction() const
	{
		return _evaluateFunction;
	}
	// Name of the Expression which is evaluated
	FName name;

private:
	EvalCB _evaluateFunction = nullptr;
};

/*
* Utility class to compile and evaluate custom C++ bool expressions
*/
class DYNAMICSTATEMACHINE_API UDSMConditionUtils
{

	using EvalCB = TFunction<bool(TWeakObjectPtr<const class UDSMDefaultNode>)>;
	using CondGrp = TMap<FName, FText>;
	using CondEvals = TArray<FExpressionEvaluator>;
	using CondValCB = TFunction<bool(FName)>;

public:
	// Validates a condition group
	// Condition groups are given as C++ style conditional expresssions
	// condNameValidation can be used to validate if names inside the C++ style conditional expresssions really exist/ can be interpreted
	// outEvaluators returns an array of executable condition evaluators
	static bool ValidateConditionGroups(const CondGrp& conditionGroups, const CondValCB& condNameValidation, CondEvals& outEvaluators);

	// Evaluates previously validated condition groups
	// ownerNode is the default Node owning all the conditions
	// evaluators are the executable condition evaluators, you receive when validating
	static bool EvaluateConditionGroups(const class UDSMDefaultNode* ownerNode, const TArray<FExpressionEvaluator>& evaluators);
protected:
	// Generates expression evaluators based on the passed condition string
	static TOptional<FExpressionEvaluator> CompileConditionString(const FName& conditionName, const FString& conditionString, const CondValCB& condNameValidation);
	// Checks if condition expression is semantically correct
	static bool ValidateConditionString(const FName& conditionName, const FString& conditionString);
	// Tokenizes the passed condition string
	static TArray<FString> TokenizeExpression(const FString& conditionString, const TArray<TCHAR>& splitOperators, const TArray<FString>& mergeOperators);
	// Convert expression from infix notation to reverse polish order, so tokens can get evaluated from left to right
	static void TransformExpressionToReversePolishOrder(TArray<FString>& tokens, const TMap<FString, int32>& operatorPrecedence);
	// Converts tokens and operations to an executable function
	static TOptional<FExpressionEvaluator> EvaluateExpression(const TArray<FString>& tokens, const CondValCB& condNameValidation);
private :
	static void CustomSplit(const FString& input, const TArray<TCHAR>& delimeters, FString& left, FString& foundDelimeter, FString& right);
	static void MergePrecedingTokens(TArray<FString>& refTokens, TArray<FString> mergeTokens);
	static EvalCB GetConditionByName(const TVariant<FString, FExpressionEvaluator>& elem, const CondValCB& condNameValidation);
	static void Print(FString contentBefore, TArray<FString> tokens);
};
