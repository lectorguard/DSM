#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "DSMDefaultNode.h"
#include "TestDataAsset.h"
#include "DSMCondition.h"


static TObjectPtr<UDSMConditionBool> CreateBoolCondition(FName dataAssetName, FName fieldName)
{
	
	TObjectPtr<UDSMConditionBool> condTrue = NewObject<UDSMConditionBool>();
	condTrue->_DataAssetName = dataAssetName;
	condTrue->_FieldName = fieldName;
	return condTrue;
}

static TObjectPtr<UDSMDefaultNode> CreateDefaultNode(TMap<FName, TObjectPtr<UDSMDataAsset>> readOnlyDataRef, TMap<FName, TObjectPtr<UDSMConditionBase>> conditionsDefs)
{
	TObjectPtr<UDSMDefaultNode> node = NewObject<UDSMDefaultNode>();
	node->_readOnlyDataReferences = readOnlyDataRef;
	node->_ConditionDefinitions = conditionsDefs;
	return node;
}

//First Parameter  : Name of the test class, has to start with an F
//Second Parameter : Description of the test case, use "." to pack tests in a namespace
//Third Parameter  : Specifies flags (At least one Filter flag (execution speed) must be set) 
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDSMValidConditionTest, "DynamicStateMachine.ValidConditions",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::ProductFilter)
	bool FDSMValidConditionTest::RunTest(const FString& Parameters) {
	// Open the map you want to test
	// 
	//AutomationOpenMap(TEXT("/TestingExamplePlugin/Maps/t_YourFirstTestMap"));
	
	TObjectPtr<UTestDataAsset> daTest = NewObject<UTestDataAsset>();
	TObjectPtr<UDSMDefaultNode> node = CreateDefaultNode({ {"daTest", daTest } }, { { "true", NewObject<UDSMConditionTrue>() },{ "false", NewObject<UDSMConditionFalse>() } });
	TestTrue("Check condition definitions is correct", node->ValidateConditionGroups());

	node->_ConditionGroups = {{ "evaluate to false", FText::FromString(TEXT("true || false && false")) }};
	TestTrue("Compiled operator ordering", node->ValidateConditionGroups());
	TestTrue("Execution operator ordering success", node->EvaluateConditionGroups());

	node->_ConditionGroups = { { "evaluate simple brackets", FText::FromString(TEXT("(true || false) && false")) } };
	TestTrue("Compiled simple brackets", node->ValidateConditionGroups());
	TestFalse("Execution simple brackets success", node->EvaluateConditionGroups());

	node->_ConditionGroups = { { "evaluate many brackets", FText::FromString(TEXT("((true && false) || true) && (true || false) && false")) } };
	TestTrue("Compiled many brackets", node->ValidateConditionGroups());
	TestFalse("Execution many brackets success", node->EvaluateConditionGroups());

	node->_ConditionGroups = { { "evaluate operator ordering long example", FText::FromString(TEXT("true && false || true && true || false && true")) } };
	TestTrue("Compiled many brackets", node->ValidateConditionGroups());
	TestTrue("Execution many brackets success", node->EvaluateConditionGroups());

	node->_ConditionGroups = { { "evaluate negation long example", FText::FromString(TEXT("!true || !(true || false)")) } };
	TestTrue("Compiled many brackets", node->ValidateConditionGroups());
	TestFalse("Execution many brackets success", node->EvaluateConditionGroups());

	node->_ConditionGroups = { { "evaluate single true example", FText::FromString(TEXT("true")) } };
	TestTrue("Compiled single true", node->ValidateConditionGroups());
	TestTrue("Execution single true", node->EvaluateConditionGroups());

	node->_ConditionGroups = { { "evaluate single false example", FText::FromString(TEXT("false")) } };
	TestTrue("Compiled single false", node->ValidateConditionGroups());
	TestFalse("Execution single false", node->EvaluateConditionGroups());

	node->_ConditionGroups = { { "evaluate single negation example", FText::FromString(TEXT("!true")) } };
	TestTrue("Compiled single negation", node->ValidateConditionGroups());
	TestFalse("Execution single negation", node->EvaluateConditionGroups());

	// Double negation is not supported
// 	node->_ConditionGroups = { { "evaluate double negation example", FText::FromString(TEXT("!!true")) } };
// 	TestTrue("Compiled double negation", node->ValidateConditionGroups());
// 	TestFalse("Execution double negation", node->EvaluateConditionGroups());

	node->_ConditionGroups = { { "evaluate negation and many brackets", FText::FromString(TEXT("!(!true || (false && true) || ((true || true) && false) || (false && false))")) } };
	TestTrue("Compiled negation and many brackets negation", node->ValidateConditionGroups());
	TestTrue("Execution negation and many brackets negation", node->EvaluateConditionGroups());

	node->_ConditionGroups =
	{	
		{ "multi condition true 1", FText::FromString(TEXT("!(!true || (false && true) || ((true || true) && false) || (false && false))")) },
		{ "multi condition true 2", FText::FromString(TEXT("true")) } ,
		{ "multi condition true 3", FText::FromString(TEXT("true || false && false")) }
	};
	TestTrue("Compiled multi conditions", node->ValidateConditionGroups());
	TestTrue("Execution multi conditions", node->EvaluateConditionGroups());

	node->_ConditionGroups =
	{
		{ "multi condition true 1", FText::FromString(TEXT("!(!true || (false && true) || ((true || true) && false) || (false && false))")) },
		{ "multi condition false 2", FText::FromString(TEXT("false")) } ,
		{ "multi condition true 3", FText::FromString(TEXT("true || false && false")) }
	};
	TestTrue("Compiled multi conditions single false", node->ValidateConditionGroups());
	TestFalse("Execution multi conditions single false", node->EvaluateConditionGroups());

	node->_ConditionGroups =
	{
		{ "multi condition false 1", FText::FromString(TEXT("(!true || (false && true) || ((true || true) && false) || (false && false))")) },
		{ "multi condition false 2", FText::FromString(TEXT("false")) } ,
		{ "multi condition false 3", FText::FromString(TEXT("!(true || false && false)")) }
	};
	TestTrue("Compiled multi conditions all false", node->ValidateConditionGroups());
	TestFalse("Execution multi conditions all false", node->EvaluateConditionGroups());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDSMInvalidConditionTest, "DynamicStateMachine.InvalidConditions",
	EAutomationTestFlags::EditorContext |
	EAutomationTestFlags::ProductFilter)
	bool FDSMInvalidConditionTest::RunTest(const FString& Parameters) {
	// Open the map you want to test
	// 
	TObjectPtr<UDSMDefaultNode> node = CreateDefaultNode({ {"daTest", nullptr } }, { { "true", CreateBoolCondition("daTest", "bTrue") },{ "false", CreateBoolCondition("daTest", "bFalse") } });
	TestFalse("Invalid data asset", node->ValidateConditionGroups());

	TObjectPtr<UTestDataAsset> daTest = NewObject<UTestDataAsset>();
	node = CreateDefaultNode({ {"daTest", daTest } }, { { "true", CreateBoolCondition("daTests", "bTrue") },{ "false", CreateBoolCondition("daTest", "bFalse") } });
	TestFalse("wrong data asset name", node->ValidateConditionGroups());

	node = CreateDefaultNode({ {"daTest", daTest } }, { { "true", CreateBoolCondition("daTest", "bTrue") },{ "false", CreateBoolCondition("daTest", "bFalses") } });
	TestFalse("wrong property name", node->ValidateConditionGroups());

	node = CreateDefaultNode({ {"daTest", daTest } }, { { "true", CreateBoolCondition("daTest", "bTrue") },{ "false", CreateBoolCondition("daTest", "bFalse") } });
	node->_ConditionGroups = { { "wrong condition usage", FText::FromString(TEXT("falses")) } };
	TestFalse("wrong usage of condition", node->ValidateConditionGroups());

	node = CreateDefaultNode({ {"daTest", daTest } }, { { "true", CreateBoolCondition("daTest", "bTrue") },{ "false", nullptr } });
	node->_ConditionGroups = { { "dummy", FText::FromString(TEXT("false")) } };
	TestFalse("condition is nullptr", node->ValidateConditionGroups());

	node = CreateDefaultNode({ {"daTest", daTest } }, { { "true", CreateBoolCondition("daTest", "bTrue") },{ "false", CreateBoolCondition("daTest", "bFalse") } });
	node->_ConditionGroups = { { "dummy", FText::FromString(TEXT("|false&")) } };
	TestFalse("wrong syntax", node->ValidateConditionGroups());

	node->_ConditionGroups = { { "dummy", FText::FromString(TEXT("| false")) } };
	TestFalse("wrong syntax", node->ValidateConditionGroups());

	node->_ConditionGroups = { { "dummy", FText::FromString(TEXT("false || && true")) } };
	TestFalse("wrong syntax", node->ValidateConditionGroups());

	node->_ConditionGroups = { { "dummy", FText::FromString(TEXT("false ( && ) true")) } };
	TestFalse("wrong syntax", node->ValidateConditionGroups());

	node->_ConditionGroups = { { "dummy", FText::FromString(TEXT("true !&& false")) } };
	TestFalse("wrong syntax", node->ValidateConditionGroups());

	node->_ConditionGroups = { { "dummy", FText::FromString(TEXT("(((true))")) } };
	TestFalse("wrong syntax", node->ValidateConditionGroups());

	node->_ConditionGroups = { { "dummy", FText::FromString(TEXT("true !false !true")) } };
	TestFalse("wrong syntax", node->ValidateConditionGroups());

	return true;
}