// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DSMDataAsset.h"
#include "Internationalization/Text.h"
#include "DSMLogInclude.h"
#include "Blueprint/UserWidget.h"
#include "Animation/AnimSequence.h"
#include "QuestConversationDataAsset.generated.h"


UCLASS(Blueprintable, DefaultToInstanced, editinlinenew)
class DYNAMICSTATEMACHINE_API UQuestBaseAction : public UObject
{
	GENERATED_BODY()
};

UCLASS(Blueprintable, DefaultToInstanced, editinlinenew)
class DYNAMICSTATEMACHINE_API UQuestMonologue : public UQuestBaseAction
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Monologue")
	FText Monologue;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Monologue")
	float Duration = 5.0f;
};

UCLASS(Blueprintable, DefaultToInstanced, editinlinenew)
class DYNAMICSTATEMACHINE_API UQuestFlipActiveQuest : public UQuestBaseAction
{
	GENERATED_BODY()
};

UCLASS(Blueprintable, DefaultToInstanced, editinlinenew)
class DYNAMICSTATEMACHINE_API UQuestFlipNeedsIntro : public UQuestBaseAction
{
	GENERATED_BODY()
};

UCLASS(Blueprintable, DefaultToInstanced, editinlinenew)
class DYNAMICSTATEMACHINE_API UQuestApplyMood : public UQuestBaseAction
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mood")
	int32 MoodIncrement = 0;
};

USTRUCT(BlueprintType)
struct DYNAMICSTATEMACHINE_API FQuestionDetail
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
		TObjectPtr<class UUserWidget> AnswerWidget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
		TObjectPtr<class UUserWidget> QuestionWidget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Question")
		FText Question;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Answer One")
		FText Answer1;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Answer One")
		TArray<TObjectPtr<UQuestBaseAction>> Answer1Actions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Answer Two")
		FText Answer2;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Answer Two")
		TArray<TObjectPtr<UQuestBaseAction>> Answer2Actions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Answer Three")
		FText Answer3;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Answer Three")
		TArray<TObjectPtr<UQuestBaseAction>> Answer3Actions;
};

UCLASS(Blueprintable, DefaultToInstanced, editinlinenew)
class DYNAMICSTATEMACHINE_API UQuestQuestion : public UQuestBaseAction
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Question")
	FQuestionDetail content;
};




/**
 * 
 */
UCLASS()
class DYNAMICSTATEMACHINE_API UQuestConversationDataAsset : public UDSMDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest Info")
	FName QuestID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest Info")
	FName GroupID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest Info")
	bool IsQuestEntry = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Actions")
	TObjectPtr<UQuestBaseAction> Introduction = nullptr;

	// Active quest can remove already performed actions
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "Actions")
	TArray<TObjectPtr<UQuestBaseAction>> Actions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Conditions")
	TArray<TSubclassOf<AActor>> InventoryCondition = {};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Conditions")
	TArray<FName> CompletedQuestCondition = {};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Conditions")
	int32 MinMoodCondition = MIN_int32;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Conditions")
	int32 MaxMoodCondition = MAX_int32;
};

UCLASS()
class DYNAMICSTATEMACHINE_API UNPCInfoAsset : public UDSMDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	int32 Mood;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	bool bQuestActive = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	TObjectPtr<UQuestConversationDataAsset> CurrentQuestInformation = nullptr;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	bool NeedsIntroduction = true;

private:
	void OnRequestDeepCopy(UObject* owner) override
	{
		if (CurrentQuestInformation)
		{
			CurrentQuestInformation = DuplicateObject<UQuestConversationDataAsset>(CurrentQuestInformation, owner);
		}
	}

};


UCLASS()
class DYNAMICSTATEMACHINE_API UPlayerInventory : public UDSMDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<TSubclassOf<AActor>> Owning = {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FName> CompletedQuestIDs{};
};


UCLASS()
class DYNAMICSTATEMACHINE_API UKeyInfo : public UDSMDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	bool HasKey = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	float DoorOpenPercent = 0.0f;
};

USTRUCT(Blueprintable)
struct DYNAMICSTATEMACHINE_API FPlayerInventoryStruct
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<TSubclassOf<AActor>> Owning = {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FName> CompletedQuestIDs{};
};


UCLASS()
class DYNAMICSTATEMACHINE_API UQuestMonologueInfo : public UDSMDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	TObjectPtr<class UUserWidget> Widget = nullptr; 	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Widget")
	float remainingDisplayTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Text")
	FString content;
};




UCLASS(Blueprintable, DefaultToInstanced, editinlinenew)
class DYNAMICSTATEMACHINE_API UQuestQuestionInfo : public UDSMDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Content")
	FQuestionDetail content;
};


UCLASS()
class DYNAMICSTATEMACHINE_API UTransformInfo : public UDSMDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info")
	FTransform transform {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info")
	TSubclassOf<AActor> type {};
};

UCLASS()
class DYNAMICSTATEMACHINE_API UDSMAnimInfo : public UDSMDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
		TObjectPtr<UAnimSequence> animationToPlay = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info")
		FName tag = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info")
		bool HasPlayed = false;
};



UCLASS(Blueprintable)
class DYNAMICSTATEMACHINE_API USaveGameInfo : public UDSMDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save Game")
	FString SlotName {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save Game")
	bool KeepState = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save Game")
	bool ClearSlotAfterLoad = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save Game")
	bool HasLoadingFinished = false;
};
