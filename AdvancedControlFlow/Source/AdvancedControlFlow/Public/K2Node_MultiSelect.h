/*!
 * AdvancedControlFlow
 *
 * Copyright (c) 2022-2023 Colory Games
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include "K2Node.h"

#include "K2Node_MultiSelect.generated.h"

 // { Case Option Pin : Case Condition Pin }
typedef TPair<UEdGraphPin*, UEdGraphPin*> CasePinPair;

UCLASS(MinimalAPI, meta = (Keywords = "Select"))
class UK2Node_MultiSelect : public UK2Node
{
	GENERATED_BODY()

	// Override from UEdGraphNode
	virtual void AllocateDefaultPins() override;
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual void PinConnectionListChanged(UEdGraphPin* Pin) override;

	// Override from UK2Node
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual bool IsNodePure() const override
	{
		return true;
	}

	// Internal functions.
	UEdGraphPin* GetOptionPinFromCaseIndex(int32 CaseIndex) const;
	UEdGraphPin* GetConditionPinFromCaseIndex(int32 CaseIndex) const;

	CasePinPair AddCasePinPair(int32 CaseIndex);
	FString GetCasePinName(const FString& Prefix, int32 CaseIndex) const;
	FString GetCasePinFriendlyName(const FString& Prefix, int32 CaseIndex) const;
	bool IsOptionPin(const UEdGraphPin* Pin) const;
	bool IsConditionPin(const UEdGraphPin* Pin) const;

	int32 GetCaseIndexFromCasePin(const FString& Prefix, UEdGraphPin* Pin) const;
	TArray<CasePinPair> GetCasePinPairs() const;
	int32 GetCasePinCount() const;

	void CreateDefaultOptionPin();
	void CreateReturnValuePin();
	UEdGraphPin* GetDefaultOptionPin() const;
	UEdGraphPin* GetReturnValuePin() const;

public:
	UK2Node_MultiSelect(const FObjectInitializer& ObjectInitializer);
};
