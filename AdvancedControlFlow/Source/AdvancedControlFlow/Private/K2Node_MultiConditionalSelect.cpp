/*!
 * AdvancedControlFlow
 *
 * Copyright (c) 2022-2023 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "K2Node_MultiConditionalSelect.h"

#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"
#include "GraphEditorSettings.h"
#include "K2Node_CallFunction.h"
#include "K2Node_MakeArray.h"
#include "K2Node_Select.h"
#include "Kismet/KismetArrayLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"

#define LOCTEXT_NAMESPACE "AdvancedControlFlow"

const FName DefaultOptionPinName(TEXT("Default"));
const FName ReturnValueOptionPinName(TEXT("Return Value"));
const FName OptionPinFriendlyNamePrefix(TEXT("Option "));
const FName ConditionPinFriendlyNamePrefix(TEXT("Condition "));

UK2Node_MultiConditionalSelect::UK2Node_MultiConditionalSelect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeContextMenuSectionName = "K2NodeMultiConditionalSelect";
	NodeContextMenuSectionLabel = LOCTEXT("MultiConditionalSelect", "Multi Conditional Select");
	CaseKeyPinNamePrefix = TEXT("CaseOption");
	CaseValuePinNamePrefix = TEXT("CaseCondition");
	CaseKeyPinFriendlyNamePrefix = TEXT("Option ");
	CaseValuePinFriendlyNamePrefix = TEXT("Condition ");
}

void UK2Node_MultiConditionalSelect::AllocateDefaultPins()
{
	// Pin structure
	//   N: Number of option/condition pin pair
	// -----
	// 0: Default (In, Wildcard)
	// 1-N: Option (In, Wildcard)
	// (N+1)-2N: Condition (In, Boolean)
	// 2N+1: Return Value (Out, Boolean)

	CreateDefaultOptionPin();
	CreateReturnValuePin();

	for (int Index = 0; Index < 2; ++Index)
	{
		AddCasePinPair(Index);
	}

	Super::AllocateDefaultPins();
}

FText UK2Node_MultiConditionalSelect::GetTooltipText() const
{
	return LOCTEXT("MultiConditionalSelect_Tooltip", "Multi-Conditional Select\nReturn the option where the condition is true");
}

FLinearColor UK2Node_MultiConditionalSelect::GetNodeTitleColor() const
{
	return GetDefault<UGraphEditorSettings>()->PureFunctionCallNodeTitleColor;
}

FText UK2Node_MultiConditionalSelect::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("MultiConditionalSelect", "Multi-Conditional Select");
}

FSlateIcon UK2Node_MultiConditionalSelect::GetIconAndTint(FLinearColor& OutColor) const
{
	static FSlateIcon Icon("EditorStyle", "GraphEditor.Select_16x");
	return Icon;
}

void UK2Node_MultiConditionalSelect::PinConnectionListChanged(UEdGraphPin* Pin)
{
	if (Pin == nullptr)
	{
		return;
	}

	if (Pin->LinkedTo.Num() == 0)
	{
		// Ignore the disconnection event.
		return;
	}

	if (IsCaseValuePin(Pin))
	{
		// Ignore condition pin connection.
		return;
	}

	if (GetDefaultOptionPin()->PinType.PinCategory != UEdGraphSchema_K2::PC_Wildcard)
	{
		// Pin type has already fixed.
		return;
	}

	Super::PinConnectionListChanged(Pin);

	Modify();

	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	UEdGraphPin* LinkedPin = Pin->LinkedTo[0];

	UEdGraphPin* DefaultOptionPin = GetDefaultOptionPin();
	DefaultOptionPin->PinType = LinkedPin->PinType;
	Schema->ResetPinToAutogeneratedDefaultValue(DefaultOptionPin);

	UEdGraphPin* ReturnValuePin = GetReturnValuePin();
	ReturnValuePin->PinType = LinkedPin->PinType;
	Schema->ResetPinToAutogeneratedDefaultValue(ReturnValuePin);

	TArray<CasePinPair> CasePinPairs = GetCasePinPairs();
	for (auto& Pair : CasePinPairs)
	{
		UEdGraphPin* OptionPin = Pair.Key;

		OptionPin->PinType = LinkedPin->PinType;
		Schema->ResetPinToAutogeneratedDefaultValue(OptionPin);
	}

	UBlueprint* Blueprint = GetBlueprint();
	FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	Blueprint->BroadcastChanged();
}

void UK2Node_MultiConditionalSelect::ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins)
{
	UEdGraphPin* OldDefaultPin = nullptr;
	for (auto& Pin : OldPins)
	{
		if (Pin->GetFName() == DefaultOptionPinName)
		{
			OldDefaultPin = Pin;
		}
	}

	CreateDefaultOptionPin();
	CreateReturnValuePin();
	Super::ReallocatePinsDuringReconstruction(OldPins);

	if (OldDefaultPin != nullptr)
	{
		GetDefaultOptionPin()->PinType = OldDefaultPin->PinType;
		GetReturnValuePin()->PinType = OldDefaultPin->PinType;
		for (auto& Pair : GetCasePinPairs())
		{
			Pair.Key->PinType = OldDefaultPin->PinType;
		}
	}
}

void UK2Node_MultiConditionalSelect::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

FText UK2Node_MultiConditionalSelect::GetMenuCategory() const
{
	return FEditorCategoryUtils::GetCommonCategory(FCommonEditorCategory::Utilities);
}

// clang-format off
/*
* Internal node structure

            +-----------------------------------------------------------------------------------------------------------------------+
            |                                                                                                                       |
            |                                         +----------------------------+           +----------------------------+       |
            |                                         |          Select            |           |           Select           |       |
            |                                         |                            |           |                            |       |
            +-+                                       +-+                        +-+           +-+                        +-+     +-+
   Option 0 | +---------------------------------------+ | Option 0  Return Value | +-----------+ | False     Return Value | +-----+ | Return Value
            +-+                                       +-+                        +-+           +-+                        +-+     +-+
            |                                         |                            |           |                            |       |
            |                                         |                            |           |                            |       |
            +-+                                       +-+                          |           +-+                          |       |
   Option 1 | +---------------------------------------+ | Option 1                 |   +-------+ | True                     |       |
            +-+                                       +-+                          |   |       +-+                          |       |
            |                                         |                            |   |       |                            |       |
            |                                         |                            |   |       |                            |       |
            +-+                                       +-+                          |   |       +-+                          |       |
    Default | +------------------+            +-------+ | Index (Integer)          |   |    +--+ | Index (Boolean)          |       |
            +-+                  |            |       +-+                          |   |    |  +-+                          |       |
            |                    |            |       |                            |   |    |  |                            |       |
            |                    |            |       |                            |   |    |  |                            |       |
            |                    |            |       +----------------------------+   |    |  +----------------------------+       |
            |                    |            x                                        |    |                                       |
            |                    |            x                                        |    |                                       |
            |                    +------------x----------------------------------------+    +------------------------+              |
            |                                 x                                                                      |              |
            |                                 x-----------------------------------+                                  |              |
            |                                                                     |                                  |              |
            |       +------------------+     +---------------------------------+  |  +-----------------------+       |              |
            |       |    Make Array    |     |          Find (Array)           |  |  |     == (Integer)      |       |              |
            |       |                  |     |                                 |  |  |                       |       |              |
            +-+     +-+              +-+     +-+                             +-+  |  +-+                   +-+       |              |
Condition 0 | +-----+ | [0]    Array | +-----+ | Target Array   Return Value | +--+--+ | A    Return Value | +-------+              |
            +-+     +-+              +-+     +-+                             +-+     +-+                   +-+                      |
            |       |                  |     |                                 |     |                       |                      |
            |       |                  |     |                                 |     |                       |                      |
            +-+     +-+                |     +-+                               |     +-+                     |                      |
Condition 1 | +-----+ | [1]            |     | | Item To Find                  |     | | B                   |                      |
            +-+     +-+                |     +-+                               |     +-+                     |                      |
            |       |                  |     |                                 |     |                       |                      |
            |       |                  |     |                                 |     |                       |                      |
            |       +------------------+     +---------------------------------+     +-----------------------+                      |
            |                                                                                                                       |
            |                                                                                                                       |
            +-----------------------------------------------------------------------------------------------------------------------+
 */
// clang-format on
void UK2Node_MultiConditionalSelect::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	UEdGraphPin* ReferenceOptionPin = GetCasePinPairs()[0].Key;
	TArray<CasePinPair> CasePinPairs = GetCasePinPairs();

	FEdGraphPinType Select1stPinType;
	Select1stPinType.PinCategory = UEdGraphSchema_K2::PC_Int;
	Select1stPinType.PinSubCategory = UEdGraphSchema_K2::PSC_Index;
	Select1stPinType.PinSubCategoryObject = nullptr;

	FEdGraphPinType Select2ndPinType;
	Select2ndPinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
	Select2ndPinType.PinSubCategory = UEdGraphSchema_K2::PSC_Index;
	Select1stPinType.PinSubCategoryObject = nullptr;

	UK2Node_Select* Select1st = CompilerContext.SpawnIntermediateNode<UK2Node_Select>(this, SourceGraph);
	Select1st->AllocateDefaultPins();
	Select1st->ChangePinType(ReferenceOptionPin);
	for (int32 Index = 2; Index < CasePinPairs.Num(); ++Index)
	{
		Select1st->AddInputPin();
	}

	UK2Node_Select* Select2nd = CompilerContext.SpawnIntermediateNode<UK2Node_Select>(this, SourceGraph);
	Select2nd->AllocateDefaultPins();
	Select2nd->ChangePinType(ReferenceOptionPin);
	for (int32 Index = 2; Index < CasePinPairs.Num(); ++Index)
	{
		Select2nd->AddInputPin();
	}

	UK2Node_MakeArray* MakeArray = CompilerContext.SpawnIntermediateNode<UK2Node_MakeArray>(this, SourceGraph);
	MakeArray->AllocateDefaultPins();
	for (int32 Index = 1; Index < CasePinPairs.Num(); ++Index)
	{
		MakeArray->AddInputPin();
	}

	UK2Node_CallFunction* ArrayFind = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	UClass* KismetArrayLibrary = UKismetArrayLibrary::StaticClass();
	UFunction* ArrayFindFunction = KismetArrayLibrary->FindFunctionByName("Array_Find");
	ArrayFind->SetFromFunction(ArrayFindFunction);
	ArrayFind->AllocateDefaultPins();

	UK2Node_CallFunction* IntEqual = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	UClass* KismetMathLibrary = UKismetMathLibrary::StaticClass();
	UFunction* EqualEqualIntIntFunction = KismetMathLibrary->FindFunctionByName("EqualEqual_IntInt");
	IntEqual->SetFromFunction(EqualEqualIntIntFunction);
	IntEqual->AllocateDefaultPins();

	// Link between outer and 1st Select
	TArray<UEdGraphPin*> Select1stOptionPins;
	Select1st->GetOptionPins(Select1stOptionPins);
	for (int Index = 0; Index < CasePinPairs.Num(); ++Index)
	{
		CompilerContext.MovePinLinksToIntermediate(*CasePinPairs[Index].Key, *Select1stOptionPins[Index]);
	}

	// Link between outer and Make Array
	TArray<UEdGraphPin*> KeyPins;
	TArray<UEdGraphPin*> ValuePins;
	MakeArray->GetKeyAndValuePins(KeyPins, ValuePins);
	for (int Index = 0; Index < CasePinPairs.Num(); ++Index)
	{
		KeyPins[Index]->PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
		CompilerContext.MovePinLinksToIntermediate(*CasePinPairs[Index].Value, *KeyPins[Index]);
	}

	// Link between Make Array and Array Find
	UEdGraphPin* ArrayPin = MakeArray->GetOutputPin();
	UEdGraphPin* TargetArrayPin = ArrayFind->FindPinChecked(TEXT("TargetArray"));
	UEdGraphPin* ItemToFindPin = ArrayFind->FindPinChecked(TEXT("ItemToFind"));
	ArrayPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
	ArrayPin->MakeLinkTo(TargetArrayPin);
	TargetArrayPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
	ItemToFindPin->PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
	MakeArray->GetSchema()->TrySetDefaultValue(*ItemToFindPin, TEXT("true"));

	// Link between Array Find and 1st Select
	UEdGraphPin* ArrayFindOutputPin = ArrayFind->GetReturnValuePin();
	UEdGraphPin* Select1stIndexPin = Select1st->GetIndexPin();
	ArrayFindOutputPin->MakeLinkTo(Select1stIndexPin);
	Select1st->NotifyPinConnectionListChanged(Select1stIndexPin);

	// Link between Array Find and Int Equal
	UEdGraphPin* IntEqualAPin = IntEqual->FindPinChecked(TEXT("A"));
	UEdGraphPin* IntEqualBPin = IntEqual->FindPinChecked(TEXT("B"));
	ArrayFindOutputPin->MakeLinkTo(IntEqualAPin);
	ArrayFindOutputPin->GetSchema()->TrySetDefaultValue(*IntEqualBPin, TEXT("-1"));

	// Link among 1st Select, 2nd Select and Int Equal
	UEdGraphPin* Select1stReturnValuePin = Select1st->GetReturnValuePin();
	UEdGraphPin* IntEqualReturnValuePin = IntEqual->GetReturnValuePin();
	UEdGraphPin* Select2ndIndexPin = Select2nd->GetIndexPin();
	TArray<UEdGraphPin*> Select2ndOptionPins;
	Select2nd->GetOptionPins(Select2ndOptionPins);
	IntEqualReturnValuePin->MakeLinkTo(Select2ndIndexPin);
	Select2nd->NotifyPinConnectionListChanged(Select2ndIndexPin);
	Select1stReturnValuePin->MakeLinkTo(Select2ndOptionPins[0]);
	CompilerContext.MovePinLinksToIntermediate(*GetDefaultOptionPin(), *Select2ndOptionPins[1]);

	// Link 2nd Select and outer
	UEdGraphPin* Select2ndReturnValuePin = Select2nd->GetReturnValuePin();
	CompilerContext.MovePinLinksToIntermediate(*GetReturnValuePin(), *Select2ndReturnValuePin);

	BreakAllNodeLinks();
}

bool UK2Node_MultiConditionalSelect::IsConnectionDisallowed(
	const UEdGraphPin* MyPin, const UEdGraphPin* OtherPin, FString& OutReason) const
{
	if (OtherPin && (OtherPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec))
	{
		OutReason = LOCTEXT("ExecConnectionDisallowd", "Can't connect with Exec pin.").ToString();
		return true;
	}

	return Super::IsConnectionDisallowed(MyPin, OtherPin, OutReason);
}

void UK2Node_MultiConditionalSelect::CreateDefaultOptionPin()
{
	FCreatePinParams Params;
	Params.Index = 0;
	UEdGraphPin* DefaultOptionPin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, DefaultOptionPinName, Params);
}

void UK2Node_MultiConditionalSelect::CreateReturnValuePin()
{
	int N = GetCasePinCount();

	FCreatePinParams Params;
	Params.Index = 2 * N + 1;
	UEdGraphPin* DefaultOptionPin = CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, ReturnValueOptionPinName, Params);
}

UEdGraphPin* UK2Node_MultiConditionalSelect::GetDefaultOptionPin() const
{
	return FindPin(DefaultOptionPinName);
}

UEdGraphPin* UK2Node_MultiConditionalSelect::GetReturnValuePin() const
{
	return FindPin(ReturnValueOptionPinName);
}

CasePinPair UK2Node_MultiConditionalSelect::AddCasePinPair(int32 CaseIndex)
{
	CasePinPair Pair;
	int N = GetCasePinCount();
	UEdGraphPin* DefaultOptionPin = GetDefaultOptionPin();

	{
		FCreatePinParams Params;
		Params.Index = 1 + CaseIndex;
		Pair.Key = CreatePin(
			EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, *GetCasePinName(CaseKeyPinNamePrefix.ToString(), CaseIndex), Params);
		Pair.Key->PinFriendlyName =
			FText::AsCultureInvariant(GetCasePinFriendlyName(CaseKeyPinFriendlyNamePrefix.ToString(), CaseIndex));
		Pair.Key->PinType = DefaultOptionPin->PinType;
	}
	{
		FCreatePinParams Params;
		Params.Index = N + 2 + CaseIndex;
		Pair.Value = CreatePin(
			EGPD_Input, UEdGraphSchema_K2::PC_Boolean, *GetCasePinName(CaseValuePinNamePrefix.ToString(), CaseIndex), Params);
		Pair.Value->PinFriendlyName =
			FText::AsCultureInvariant(GetCasePinFriendlyName(CaseValuePinFriendlyNamePrefix.ToString(), CaseIndex));
	}

	return Pair;
}

#undef LOCTEXT_NAMESPACE
