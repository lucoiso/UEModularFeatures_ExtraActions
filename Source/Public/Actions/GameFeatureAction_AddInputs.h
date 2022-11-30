// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#pragma once

#include <CoreMinimal.h>
#include <InputTriggers.h>
#include <EnhancedInputComponent.h>
#include <GameplayAbilitySpec.h>
#include "Actions/GameFeatureAction_WorldActionBase.h"
#include "GameFeatureAction_AddInputs.generated.h"

class UGameplayAbility;
class UInputMappingContext;
class UEnhancedInputLocalPlayerSubsystem;
struct FComponentRequestHandle;

/**
 *
 */

USTRUCT(BlueprintType, Category = "MF Extra Actions | Modular Structs")
struct FFunctionStackedData
{
	GENERATED_BODY()

	/* UFunction name */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FName FunctionName;

	/* Input Trigger event type */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TArray<ETriggerEvent> Triggers;
};

/* Settings for ability binding: Use InputID Value Name if Ability Binding Mode is InputID | Use AbilityTags if Ability Binding Mode is AbilityTags | Use AbilityClass if Ability Binding Mode is AbilityClass | Use bFindAbilitySpec and AbilityClass if Ability Binding Mode is AbilitySpec */
USTRUCT(BlueprintType, Category = "MF Extra Actions | Modular Structs")
struct FAbilityInputBindingData
{
	GENERATED_BODY()

	/* Should this action setup call SetupAbilityInput/RemoveAbilityInputBinding using the IMFEA_AbilityInputBinding interface? */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	bool bSetupAbilityInput = false;

	/* Bind this input to an ability activation using the Input ID - If binding mode is set to InputID */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (DisplayName = "InputID Value Name", EditCondition = "bSetupAbilityInput"))
	FName InputIDValueName = NAME_None;

	/* Bind this input to an ability activation using Ability Tags container - If binding mode is set to Ability Tags */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (EditCondition = "bSetupAbilityInput"))
	FGameplayTagContainer AbilityTags = FGameplayTagContainer::EmptyContainer;

	/* Bind this input to an ability activation using the Ability Class - If binding mode is set to Ability Class */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (EditCondition = "bSetupAbilityInput"))
	TSoftClassPtr<UGameplayAbility> AbilityClass;

	/* Bind this input to an ability activation using the active ability spec - If binding mode is set to Ability Spec - Must specify the Ability Class! */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (EditCondition = "bSetupAbilityInput && AbilityClass != nullptr"))
	bool bFindAbilitySpec = false;
};

USTRUCT(BlueprintType, Category = "MF Extra Actions | Modular Structs")
struct FInputMappingStack
{
	GENERATED_BODY()

	/* Enhanced Input Action to bind with these settings */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TSoftObjectPtr<UInputAction> ActionInput;

	/* Settings to bind this Action Input to a Ability System Component */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FAbilityInputBindingData AbilityBindingData;

	/* UFunction and Triggers to bind activation by Enhanced Input */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (DisplayName = "UFunction Bindings"))
	TArray<FFunctionStackedData> FunctionBindingData;
};

/**
 *
 */
UCLASS(MinimalAPI, meta = (DisplayName = "MF Extra Actions: Add Enhanced Input Mapping"))
class UGameFeatureAction_AddInputs final : public UGameFeatureAction_WorldActionBase
{
	GENERATED_BODY()

public:
	/* Target pawn to which Input Mapping will be given */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowedClasses = "/Script/Engine.Pawn", OnlyPlaceable = "true"))
	TSoftClassPtr<APawn> TargetPawnClass;

	/* Determines whether the binding will be performed within the controller class or within the pawn */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	EInputBindingOwnerOverride InputBindingOwnerOverride = EInputBindingOwnerOverride::Default;

	/* Tags required on the target to apply this action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TArray<FName> RequireTags;

	/* Enhanced Input Mapping Context to be added */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TSoftObjectPtr<UInputMappingContext> InputMappingContext;

	/* Input Mapping priority */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	int32 MappingPriority = 1;

	/* Enhanced Input Actions binding stacked data */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (DisplayName = "Actions Bindings", ShowOnlyInnerProperties))
	TArray<FInputMappingStack> ActionsBindings;

protected:
	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
	virtual void AddToWorld(const FWorldContext& WorldContext) override;

private:
	void HandleActorExtension(AActor* Owner, FName EventName);
	void ResetExtension();

	void AddActorInputs(AActor* TargetActor);
	void RemoveActorInputs(AActor* TargetActor);

	void SetupActionBindings(AActor* TargetActor, UObject* FunctionOwner, UEnhancedInputComponent* InputComponent);

	FGameplayAbilitySpec GetAbilitySpecInformationFromBindingData(AActor* TargetActor, const FAbilityInputBindingData& AbilityBindingData, UEnum* InputIDEnum = nullptr);
	UEnhancedInputLocalPlayerSubsystem* GetEnhancedInputComponentFromPawn(APawn* TargetPawn);

	struct FInputBindingData
	{
		TArray<FInputBindingHandle> ActionBinding;
		TWeakObjectPtr<UInputMappingContext> Mapping;
	};

	TMap<TWeakObjectPtr<AActor>, FInputBindingData> ActiveExtensions;
	TArray<TWeakObjectPtr<UInputAction>> AbilityActions;
};
