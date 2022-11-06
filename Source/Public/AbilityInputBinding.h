// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayAbilitySpec.h"
#include "GameplayTagContainer.h"
#include "AbilityInputBinding.generated.h"

/**
 *
 */
class UInputAction;

/* Your pawn or controller need this inferface to accept ability input bindings */
UINTERFACE(MinimalAPI, Blueprintable, Category = "MF Extra Actions | Modular Interfaces")
class UAbilityInputBinding : public UInterface
{
	GENERATED_BODY()
};

/* Your pawn or controller need this inferface to accept ability input bindings */
class MODULARFEATURES_EXTRAACTIONS_API IAbilityInputBinding
{
	GENERATED_IINTERFACE_BODY()
		
	/* This function is needed for setup ability input binding inside your controller or pawn */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "MF Extra Actions | Modular Interfaces", Meta = (Deprecated = "true", DeprecationMessage = "There are new functions to bind the input. Please check SetupAbilityBindingByInput, SetupAbilityBindingBySpec, SetupAbilityBindingByTags and SetupAbilityBindingByClass"))
	void SetupAbilityInputBinding(UInputAction* Action, const int32 InputID);

	/* Setup ability bind using input id - AbilityLocalInputPressed(InputID)*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "MF Extra Actions | Modular Interfaces")
	void SetupAbilityBindingByInput(UInputAction* Action, const int32 InputID);

	/* Setup ability bind using the ability spec - AbilitySpecInputPressed(AbilitySpec) */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "MF Extra Actions | Modular Interfaces")
	void SetupAbilityBindingBySpec(UInputAction* Action, FGameplayAbilitySpec& AbilitySpec);

	/* Setup ability bind using the ability tags container - TryActivateAbilitiesByTag(AbilityTags) */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "MF Extra Actions | Modular Interfaces")
	void SetupAbilityBindingByTags(UInputAction* Action, const FGameplayTagContainer& AbilityTags);

	/* Setup ability bind using the ability tags container - TryActivateAbilityByClass(AbilityClass) */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "MF Extra Actions | Modular Interfaces")
	void SetupAbilityBindingByClass(UInputAction* Action, TSubclassOf<UGameplayAbility> AbilityClass);

	/* This function is needed for removing ability input binding inside your controller or pawn */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "MF Extra Actions | Modular Interfaces")
	void RemoveAbilityInputBinding(const UInputAction* Action);
};
