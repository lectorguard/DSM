// Fill out your copyright notice in the Description page of Project Settings.


#include "DSMConditionUtils.h"
#include "DSMDefaultNode.h"
#include "DSMCondition.h"

bool UDSMConditionUtils::EvaluateConditionGroups(const class UDSMDefaultNode* ownerNode, const TArray<FExpressionEvaluator>& evaluators)
{
	bool isValid = true;
	for (int32 i = 0; i < evaluators.Num(); ++i)
	{
		const bool result = evaluators[i].GetEvaluateFunction()(ownerNode);
		if (ownerNode)
		{
			UE_LOG(LogDSM, Log, TEXT("Node %s (outer %s) : Condition result : %s with name %s "),
				*ownerNode->GetName(),
				*ownerNode->GetOuter()->GetName(),
				*(result ? FString("true") : FString("false")),
				* evaluators[i].name.ToString());
		}
		isValid = isValid && result;
		
	}
	return isValid;
}

bool UDSMConditionUtils::ValidateConditionGroups(const CondGrp& conditionGroups, const CondValCB& condNameValidation, CondEvals& outEvaluators)
{
	TArray<FExpressionEvaluator> validExpressions;
	for (const TPair<FName, FText>& pair : conditionGroups)
	{
		if (TOptional<FExpressionEvaluator> expression = CompileConditionString(pair.Key, pair.Value.ToString(), condNameValidation))
		{
			expression->name = pair.Key;
			validExpressions.Add(*expression);
		}
		else
		{
			return false;
		}
	}
	outEvaluators = validExpressions;
	return true;
}

UDSMConditionUtils::EvalCB UDSMConditionUtils::GetConditionByName(const TVariant<FString, FExpressionEvaluator>& elem, const CondValCB& condNameValidation)
{
	if (elem.IsType<FString>())
	{
		const FString conditionName = elem.Get<FString>();
		if (conditionName.IsEmpty() || !condNameValidation(FName(*conditionName)))
		{
			return nullptr;
		}

		return [conditionName](TWeakObjectPtr<const UDSMDefaultNode> node)
		{
			if (node.IsValid())
			{
				const TWeakObjectPtr<UDSMConditionBase> condition = node->_ConditionDefinitions[FName(*conditionName)];
				if (condition.IsValid())
				{
					if (condition->bIsBound)
					{
						return condition->Evaluate(node);
					}
					else
					{
						UE_LOG(LogDSM, Warning, TEXT("Condition %s is not bound. Plaese bind it before evaluation."), *conditionName);
					}
				}
			} 
			return false;
		};
	}
	else
	{
		return elem.Get<FExpressionEvaluator>().GetEvaluateFunction();
	}
}


TOptional<FExpressionEvaluator> UDSMConditionUtils::CompileConditionString(const FName& conditionName, const FString& conditionString, const CondValCB& condNameValidation)
{
	// Tokenize infix notation expression
	if (ValidateConditionString(conditionName, conditionString))
	{
		TArray<FString> tokens = TokenizeExpression(conditionString, { '(' , ')', '|', '&', '!'}, {"&", "|"});
		Print("Generated infix tokenization of expression : ", tokens);
		// Convert tokens to reverse polish order to get rid of brackets btw. correct evaluation order
		TransformExpressionToReversePolishOrder(tokens, { {"||", 0}, {"&&", 1}, {"!", 2}});
		Print("Converted tokens to reverse polish order : ", tokens);
		// Create evaluation object
		return EvaluateExpression(tokens, condNameValidation);
	}
	return TOptional<FExpressionEvaluator>();
}

bool UDSMConditionUtils::ValidateConditionString(const FName& conditionName, const FString& conditionString)
{
	// Allowed variable chars can contain 0-9, a-z, and A-Z
	const TArray<TCHAR, TInlineAllocator<62>> allowedCharsForVariables = {
						'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', // numbers 0-9
						'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
						'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
						'u', 'v', 'w', 'x', 'y', 'z', // lowercase letters
						'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
						'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
						'U', 'V', 'W', 'X', 'Y', 'Z' // uppercase letters
	};

	int32 bracketsCounter = 0;
	bool bIsVariableToTheLeft = false;
	bool bNeedVariableToTheRight = false;
	for (int32 i = 0; i < conditionString.Len(); ++i)
	{
		const TCHAR currentChar = conditionString[i];
		switch (currentChar)
		{
		case '(':
			++bracketsCounter;
			break;
		case ')':
			--bracketsCounter;
			break;
		case '!':
		{
			// Character to the left must be a space, if i > 0
			if (i > 0 && conditionString[i - 1] != ' ' && conditionString[i - 1] != '(')
			{
				UE_LOG(LogDSM, Warning, TEXT("Condition %s contains wrong usage of negation character {!}"), *conditionName.ToString());
				UE_LOG(LogDSM, Warning, TEXT("Character to the left must be a space"), *conditionName.ToString());
				return false;
			}
			// Character to the right must be a variableChar
			if (i > conditionString.Len() - 2 || conditionString[i + 1] == '|' || conditionString[i + 1] == '&')
			{
				UE_LOG(LogDSM, Warning, TEXT("Condition %s contains wrong usage of negation character {!}"), *conditionName.ToString());
				UE_LOG(LogDSM, Warning, TEXT("Character to the right can not be && or ||"), *conditionName.ToString());
				return false;
			}

			break;
		}
		case '|':
		case '&':
		{
			// Check if there is a variable to the left
			if (!bIsVariableToTheLeft)
			{
				UE_LOG(LogDSM, Warning, TEXT("Condition %s needs a variable left from char {%c}"), *conditionName.ToString(), currentChar);
				return false;
			}
			else
			{
				bIsVariableToTheLeft = false;
			}
			bNeedVariableToTheRight = true;

			// Character to the left must be a space, if i > 0
			if (i != 0 && conditionString[i - 1] != ' ')
			{
				UE_LOG(LogDSM, Warning, TEXT("Condition %s contains wrong usage of character {%c}"), *conditionName.ToString(), currentChar);
				UE_LOG(LogDSM, Warning, TEXT("Character to the left must be a space"), *conditionName.ToString());
				return false;
			}
			// Character to the right must be of same type
			if (i <= conditionString.Len() - 3 && conditionString[i + 1] == conditionString[i])
			{
				++i;
			}
			else
			{
				UE_LOG(LogDSM, Warning, TEXT("Condition %s contains wrong usage of character {%c}"), *conditionName.ToString(), currentChar);
				UE_LOG(LogDSM, Warning, TEXT("Character can only be used in pairs"), *conditionName.ToString());
				return false;
			}
			// Character to the right must be a space, if i > 0
			if (conditionString[i + 1] != ' ')
			{
				UE_LOG(LogDSM, Warning, TEXT("Condition %s contains wrong usage of character {%c}"), *conditionName.ToString(), currentChar);
				UE_LOG(LogDSM, Warning, TEXT("Character to the right must be a space"), *conditionName.ToString());
				return false;
			}
			break;
		}
		case ' ':
			break;
		default:
		{
			// Check if it is a valid character for variable
			if (!allowedCharsForVariables.Contains(currentChar))
			{
				UE_LOG(LogDSM, Warning, TEXT("Condition %s contains invalid character : {%c}"), *conditionName.ToString(), currentChar);
				return false;
			}
			// Update variables to the left and right
			if (bNeedVariableToTheRight)
			{
				bNeedVariableToTheRight = false;
			}
			bIsVariableToTheLeft = true;
			break;
		}
		}
	}
	if (bracketsCounter != 0)
	{
		UE_LOG(LogDSM, Warning, TEXT("Condition %s contains uneven number of opening and closing brackets"), *conditionName.ToString());
		return false;
	}
	if (bNeedVariableToTheRight)
	{
		UE_LOG(LogDSM, Warning, TEXT("Condition %s is missing a variable to the right next to {&&} or {||} character pair"), *conditionName.ToString());
		return false;
	}
	// Syntax ok
	return true;
}

TArray<FString> UDSMConditionUtils::TokenizeExpression(const FString& conditionString, const TArray<TCHAR>& splitOperators, const TArray<FString>& mergeOperators)
{
	TArray<FString> tokens = {};
	FString remainingString = conditionString;
	remainingString.RemoveSpacesInline();
	while (!remainingString.IsEmpty())
	{
		FString left;
		FString delimeter;
		CustomSplit(remainingString, splitOperators, left, delimeter, remainingString);
		if (!left.IsEmpty())
		{
			tokens.Add(left);
		}
		if (!delimeter.IsEmpty())
		{
			tokens.Add(delimeter);
		}
	}
	// Merge preceding tokens &,|
	MergePrecedingTokens(tokens, mergeOperators);
	return tokens;
}

void UDSMConditionUtils::TransformExpressionToReversePolishOrder(TArray<FString>& tokens, const TMap<FString, int32>& operatorPrecedence)
{
	// Converts the expression from infix notation into reverse polish order (RPO)
	const TArray<FString> parenthesis = { "(", ")" };
	TArray<FString> definedOperators;
	operatorPrecedence.GetKeys(definedOperators);
	TArray<FString> operators = {};
	TArray<FString> result = {};
	for (const FString& token : tokens)
	{
		if (token.Equals("("))
		{
			operators.Add(token);
		}
		// Add all operators in bracket to the result
		else if (token.Equals(")"))
		{
			while (!operators.Last().Equals("("))
			{
				result.Push(operators.Last());
				operators.Pop();
			}
			operators.Pop();
		}
		// Manage operators, no special order must be handled because AND and OR operator have same priority
		else if (definedOperators.Contains(token))
		{
			// If presendence of last operator is higher than current one and the last one is not a parenthesis then you can unwind
			// Rule && > ||
			while (!operators.IsEmpty() &&
				!parenthesis.Contains(operators.Last()) &&
				operatorPrecedence[operators.Last()] >= operatorPrecedence[token])
			{
				result.Push(operators.Last());
				operators.Pop();
			}
			operators.Push(token);
		}
		// Must be variable
		else
		{
			result.Push(token);
		}
	}
	//Push remaining opertor
	for (int32 i = operators.Num() - 1; i >= 0; --i)
	{
		result.Push(operators[i]);
	}
	tokens = result;
}

TOptional<FExpressionEvaluator> UDSMConditionUtils::EvaluateExpression(const TArray<FString>& tokens, const CondValCB& condNameValidation)
{
	TArray<TVariant<FString, FExpressionEvaluator>> evaluationStack = {};
	// single variable, just return it
	if (tokens.Num() == 1)
	{
		TVariant<FString, FExpressionEvaluator> token;
		token.Set<FString>(tokens[0]);
		if (const EvalCB cb = GetConditionByName(token, condNameValidation))
		{
			// Expression evaluator with  token || false 
			FExpressionEvaluator evaluator{ cb, [](auto a) {return false; }, false };
			return evaluator;
		}
		else
		{
			return TOptional<FExpressionEvaluator>();
		}
	}

	for (const FString& token : tokens)
	{
		TVariant<FString, FExpressionEvaluator> newElement;
		// NOT operator
		if (token.Equals("!"))
		{
			if (evaluationStack.Num() < 1)
			{
				UE_LOG(LogDSM, Warning, TEXT("Expression can not be evaluated, expect variable token/ or token group when using !"));
				return TOptional<FExpressionEvaluator>();
			}
			if (EvalCB cb = GetConditionByName(evaluationStack.Pop(), condNameValidation))
			{
				FExpressionEvaluator evaluator{ cb };
				newElement.Set<FExpressionEvaluator>(evaluator);
				evaluationStack.Push(newElement);
			}
			else
			{
				return TOptional<FExpressionEvaluator>();
			}
		} 
		// AND or OR operator 
		else if (token.Equals("||") || token.Equals("&&"))
		{
			if (evaluationStack.Num() < 2)
			{
				UE_LOG(LogDSM, Warning, TEXT("Expression can not be evaluated, AND and OR statements need a variable to its left and right"));
				return TOptional<FExpressionEvaluator>();
			}
			// If there is an operator, we pop the last 2 values from the stack and 
			// Create an expression evaluator with the operator and the values
			// Afterwards we push the evaluator back to the evaluation stack
			TArray<EvalCB, TInlineAllocator<2>> evaluationElements = {};
			for (int32 i = 0; i < 2; ++i)
			{
				if (EvalCB cb = GetConditionByName(evaluationStack.Pop(), condNameValidation))
				{
					evaluationElements.Add(cb);
				}
				else
				{
					return TOptional<FExpressionEvaluator>();
				}
			}
			const bool bIsAnd = token == "&&";
			FExpressionEvaluator evaluator{ evaluationElements[1], evaluationElements[0], bIsAnd };
			newElement.Set<FExpressionEvaluator>(evaluator);
			evaluationStack.Push(newElement);
		}
		// Must be variable
		else
		{
			// We push all found variables to the stack
			newElement.Set<FString>(token);
			evaluationStack.Push(newElement);
		}
	}

	if (evaluationStack.Num() > 1)
	{
		UE_LOG(LogDSM, Warning, TEXT("Error happend when evaluating the expression, too many remaining tokens"));
		return TOptional<FExpressionEvaluator>();;
	}
	else if (evaluationStack.Num() == 1)
	{
		return evaluationStack.Pop().Get<FExpressionEvaluator>();
	}
	else
	{
		return TOptional<FExpressionEvaluator>();
	}
}


void UDSMConditionUtils::MergePrecedingTokens(TArray<FString>& refTokens, TArray<FString> mergeTokens)
{
	if (refTokens.IsEmpty())
	{
		return;
	}

	TArray<FString> mergedTokens = { refTokens[0] };
	FString currentToken = refTokens[0];
	for (int32 i = 1; i < refTokens.Num(); ++i)
	{
		if (mergeTokens.Contains(currentToken) &&
			currentToken.Equals(refTokens[i]))
		{
			mergedTokens.Last().Append(refTokens[i]);
		}
		else
		{
			currentToken = refTokens[i];
			mergedTokens.Add(refTokens[i]);
		}
	}
	refTokens = mergedTokens;
}

void UDSMConditionUtils::CustomSplit(const FString& input, const TArray<TCHAR>& delimeters, FString& left, FString& foundDelimeter, FString& right)
{
	for (int32 i = 0; i < input.Len(); ++i)
	{
		const TCHAR currentChar = input[i];
		if (delimeters.Contains(currentChar))
		{
			left = i > 0 ? input.Mid(0, i) : "";
			foundDelimeter = FString::Printf(TEXT("%c"), currentChar);
			right = i+1 < input.Len() ? input.Mid(i + 1) : "";
			return;
		}
	}
	left = input;
	foundDelimeter = "";
	right = "";
}

void UDSMConditionUtils::Print(FString contentBefore, TArray<FString> tokens)
{
	for (auto token : tokens)
	{
		contentBefore.Append(token);
		contentBefore.Append(",");
	}
	UE_LOG(LogDSM, Log, TEXT("Infix notation of tokenized expression : %s"), *contentBefore);
}