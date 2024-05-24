// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#include "Actions/GameFeatureAction_AddInputs.h"
#include "ModularFeatures_InternalFuncs.h"
#include <EnhancedInputSubsystems.h>
#include <InputMappingContext.h>
#include <Components/GameFrameworkComponentManager.h>
#include <GameFramework/PlayerController.h>
#include <Engine/LocalPlayer.h>

#ifdef UE_INLINE_GENERATED_CPP_BY_NAME
#include UE_INLINE_GENERATED_CPP_BY_NAME(GameFeatureAction_AddInputs)
#endif

void UGameFeatureAction_AddInputs::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	if (!ensureAlways(ActiveExtensions.IsEmpty()))
	{
		ResetExtension();
	}

	Super::OnGameFeatureActivating(Context);
}

void UGameFeatureAction_AddInputs::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);
	ResetExtension();
}

void UGameFeatureAction_AddInputs::ResetExtension()
{
	while (!ActiveExtensions.IsEmpty())
	{
		const auto ExtensionIterator = ActiveExtensions.CreateConstIterator();
		RemoveActorInputs(ExtensionIterator->Key.Get());
	}

	Super::ResetExtension();
}

void UGameFeatureAction_AddInputs::AddToWorld(const FWorldContext& WorldContext)
{
	if (UGameFrameworkComponentManager* const ComponentManager = GetGameFrameworkComponentManager(WorldContext); IsValid(ComponentManager) && !
		TargetPawnClass.IsNull())
	{
		using FHandlerDelegate = UGameFrameworkComponentManager::FExtensionHandlerDelegate;
		const FHandlerDelegate ExtensionHandlerDelegate = FHandlerDelegate::CreateUObject(this, &UGameFeatureAction_AddInputs::HandleActorExtension);

		ActiveRequests.Add(ComponentManager->AddExtensionHandler(TargetPawnClass, ExtensionHandlerDelegate));
	}
}

void UGameFeatureAction_AddInputs::HandleActorExtension(AActor* Owner, const FName EventName)
{
	if (EventName == UGameFrameworkComponentManager::NAME_ExtensionRemoved || EventName == UGameFrameworkComponentManager::NAME_ReceiverRemoved)
	{
		RemoveActorInputs(Owner);
	}

	else if (EventName == UGameFrameworkComponentManager::NAME_ExtensionAdded || EventName == UGameFrameworkComponentManager::NAME_GameActorReady)
	{
		// We don't want to repeat the addition and cannot add if the user don't have the required tags
		if (ActiveExtensions.Contains(Owner) || !ModularFeaturesHelper::ActorHasAllRequiredTags(Owner, RequireTags))
		{
			return;
		}

		if (InputMappingContext.IsNull())
		{
			UE_LOG(LogGameplayFeaturesExtraActions_Internal, Error, TEXT("%s: Input Mapping Context is null."), *FString(__FUNCTION__));
		}
		else
		{
			AddActorInputs(Owner);
		}
	}
}

void UGameFeatureAction_AddInputs::AddActorInputs(AActor* TargetActor)
{
	// Only proceed if the target actor is valid
	if (!IsValid(TargetActor))
	{
		return;
	}

	// Get the target pawn and check if it is valid. We don't want to add inputs to a non-pawn actor
	APawn* const TargetPawn = Cast<APawn>(TargetActor);
	if (!IsValid(TargetPawn))
	{
		return;
	}

	// Try to get the enhanced input subsystem from the pawn
	if (UEnhancedInputLocalPlayerSubsystem* const Subsystem = GetEnhancedInputComponentFromPawn(TargetPawn))
	{
		// Cehck if there's already an existing input data associated to the target actor and get it or create a new one
		FInputBindingData& NewInputData = ActiveExtensions.FindOrAdd(TargetActor);

		// Load the Input Mapping context and store it into a variable
		UInputMappingContext* const InputMapping = InputMappingContext.LoadSynchronous();

		UE_LOG(LogGameplayFeaturesExtraActions_Internal, Display, TEXT("%s: Adding Enhanced Input Mapping %s to Actor %s."), *FString(__FUNCTION__),
		       *InputMapping->GetName(), *TargetActor->GetName());

		// Add the loaded mapping context into the enhanced input subsystem
		Subsystem->AddMappingContext(InputMapping, MappingPriority);

		// Add the mapping context to the input data
		NewInputData.Mapping = InputMapping;

		// Get the Function Owner, the UObject which owns the specified UFunction that will be used to bind the input activation and check if it's valid
		const TWeakObjectPtr<UObject> FunctionOwner = ModularFeaturesHelper::GetPawnInputOwner(TargetPawn, InputBindingOwnerOverride);
		if (!FunctionOwner.IsValid())
		{
			UE_LOG(LogGameplayFeaturesExtraActions_Internal, Error, TEXT("%s: Failed to get the function owner using the Actor %s."),
			       *FString(__FUNCTION__), *TargetActor->GetName());
			return;
		}

		// Get the Enhanced Input component of the target Pawn and check if it's valid
		const TWeakObjectPtr<UEnhancedInputComponent> InputComponent = ModularFeaturesHelper::GetEnhancedInputComponentInPawn(TargetPawn);
		if (!InputComponent.IsValid())
		{
			UE_LOG(LogGameplayFeaturesExtraActions_Internal, Error, TEXT("%s: Failed to find InputComponent on Actor %s."), *FString(__FUNCTION__),
			       *TargetActor->GetName());
			return;
		}

		// If everything is okay, setup the action bindings and add the extension to the active map
		SetupActionBindings(TargetActor, FunctionOwner.Get(), InputComponent.Get());
		ActiveExtensions.Add(TargetActor, NewInputData);
	}
	else if (TargetPawn->IsPawnControlled())
	{
		UE_LOG(LogGameplayFeaturesExtraActions_Internal, Error, TEXT("%s: Failed to find PlayerController on Actor %s."), *FString(__FUNCTION__),
		       *TargetActor->GetName());
	}
}

void UGameFeatureAction_AddInputs::RemoveActorInputs(AActor* TargetActor)
{
	// Only proceed if the target actor is valid
	if (!IsValid(TargetActor))
	{
		ActiveExtensions.Remove(TargetActor);
		return;
	}

	// Get the target pawn and check if it is valid. We don't want to check inputs from a non-pawn actor
	APawn* const TargetPawn = Cast<APawn>(TargetActor);
	if (!IsValid(TargetPawn))
	{
		ActiveExtensions.Remove(TargetActor);
		return;
	}

	// Check if there's existing active input data
	if (const FInputBindingData* const ActiveInputData = ActiveExtensions.Find(TargetActor))
	{
		// Try to get the enhanced input subsystem from the pawn
		if (UEnhancedInputLocalPlayerSubsystem* const Subsystem = GetEnhancedInputComponentFromPawn(TargetPawn))
		{
			UE_LOG(LogGameplayFeaturesExtraActions_Internal, Display, TEXT("%s: Removing Enhanced Input Mapping %s from Actor %s."),
			       *FString(__FUNCTION__), *ActiveInputData->Mapping->GetName(), *TargetActor->GetName());

			// Try to get the enhanced input component of the target pawn
			if (const TWeakObjectPtr<UEnhancedInputComponent> InputComponent = ModularFeaturesHelper::GetEnhancedInputComponentInPawn(TargetPawn); !
				InputComponent.IsValid())
			{
				UE_LOG(LogGameplayFeaturesExtraActions_Internal, Error, TEXT("%s: Failed to find InputComponent on Actor %s."), *FString(__FUNCTION__),
				       *TargetActor->GetName());
			}
			else
			{
				// Iterate through the active bindings and remove all
				for (const FInputBindingHandle& InputActionBinding : ActiveInputData->ActionBinding)
				{
					InputComponent->RemoveBinding(InputActionBinding);
				}

				// Verify and try to remove the ability bindings by calling the RemoveAbilityInputBinding from IMFEA_AbilityInputBinding interface
				if (const IMFEA_AbilityInputBinding* const SetupInputInterface = ModularFeaturesHelper::GetAbilityInputBindingInterface(
					TargetActor, InputBindingOwnerOverride))
				{
					ModularFeaturesHelper::RemoveAbilityInputInInterfaceOwner(SetupInputInterface->_getUObject(), AbilityActions);
				}
			}

			// Remove the mapping context from the subsystem
			Subsystem->RemoveMappingContext(ActiveInputData->Mapping.Get());
		}
	}

	ActiveExtensions.Remove(TargetActor);
}

void UGameFeatureAction_AddInputs::SetupActionBindings(AActor* TargetActor, UObject* FunctionOwner, UEnhancedInputComponent* InputComponent)
{
	// Get the existing input data
	FInputBindingData& NewInputData = ActiveExtensions.FindOrAdd(TargetActor);

	// Get and store the interface owner before the for-loop
	const IMFEA_AbilityInputBinding* const SetupInputInterface = ModularFeaturesHelper::GetAbilityInputBindingInterface(
		TargetActor, InputBindingOwnerOverride);

	// Load the enumeration and save it before the loop to avoid high disk consumption due to loading a soft reference a lot of times since there's only 1 enumeration
	const TWeakObjectPtr<UEnum> InputIDEnumeration_Ptr = ModularFeaturesHelper::LoadInputEnum();

	// Iterate through the bindings to add all of them
	for (const auto& [ActionInput, AbilityBindingData, FunctionBindingData] : ActionsBindings)
	{
		// Check if the action input is valid
		if (ActionInput.IsNull())
		{
			UE_LOG(LogGameplayFeaturesExtraActions_Internal, Error, TEXT("%s: Action Input is null."), *FString(__FUNCTION__));
			continue;
		}

		// Load and store the Action Input into a variable
		UInputAction* const InputAction = ActionInput.LoadSynchronous();

		UE_LOG(LogGameplayFeaturesExtraActions_Internal, Display, TEXT("%s: Binding Action Input %s to Actor %s."), *FString(__FUNCTION__),
		       *InputAction->GetName(), *TargetActor->GetName());

		// Iterate through all function binding data to bind the UFunctions to it's corresponding input
		for (const auto& [FunctionName, Triggers] : FunctionBindingData)
		{
			// Iterate through all triggersto bind the UFunctions to it's corresponding trigger type
			for (const ETriggerEvent& Trigger : Triggers)
			{
				NewInputData.ActionBinding.Add(InputComponent->BindAction(InputAction, Trigger, FunctionOwner, FunctionName));
			}
		}

		// Check if this input will be used to setup existing abilities
		if (!AbilityBindingData.bSetupAbilityInput)
		{
			continue;
		}

		// Try to bind this input by calling the function SetupAbilityInputBinding from IMFEA_AbilityInputBinding interface
		if (FGameplayAbilitySpec NewAbilitySpec = GetAbilitySpecInformationFromBindingData(
			TargetActor, AbilityBindingData, InputIDEnumeration_Ptr.Get()); ModularFeaturesHelper::BindAbilityInputToInterfaceOwner(
			SetupInputInterface, InputAction, NewAbilitySpec))
		{
			AbilityActions.Add(InputAction);
		}
	}
}

UEnhancedInputLocalPlayerSubsystem* UGameFeatureAction_AddInputs::GetEnhancedInputComponentFromPawn(APawn* TargetPawn)
{
	// Get the Player Controller associated to this pawn
	if (const APlayerController* const PlayerController = TargetPawn->GetController<APlayerController>())
	{
		// Check if the controller is local. We don't want to add inputs to a non-local player
		if (!PlayerController->IsLocalController())
		{
			return nullptr;
		}

		// Get the local player associated to the controller
		if (const ULocalPlayer* const LocalPlayer = PlayerController->GetLocalPlayer())
		{
			// Get the enhanced input subsystem of the local player and check if its valid
			UEnhancedInputLocalPlayerSubsystem* const Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
			if (!IsValid(Subsystem))
			{
				UE_LOG(LogGameplayFeaturesExtraActions_Internal, Error, TEXT("%s: LocalPlayer %s has no EnhancedInputLocalPlayerSubsystem."),
				       *FString(__FUNCTION__), *LocalPlayer->GetName());
				return nullptr;
			}

			return Subsystem;
		}
	}

	return nullptr;
}

FGameplayAbilitySpec UGameFeatureAction_AddInputs::GetAbilitySpecInformationFromBindingData(
	AActor* TargetActor, const FAbilityInputBindingData& AbilityBindingData, UEnum* InputIDEnum)
{
	// Get the ability input ID from the enumeration by its value name
	const int32 InputID = ModularFeaturesHelper::GetInputIDByName(AbilityBindingData.InputIDValueName, InputIDEnum);

	// Create the spec, used as param of ability binding
	FGameplayAbilitySpec NewAbilitySpec;

	// If the user wants to find a active ability spec, we'll try to get the ability system component of the target actor and get the spec using the specified ability class
	if (AbilityBindingData.bFindAbilitySpec)
	{
		if (const UAbilitySystemComponent* const AbilitySystemComponent = ModularFeaturesHelper::GetAbilitySystemComponentInActor(TargetActor))
		{
			// We're not using the InputID to search for existing spec because more than 1 abilities can have the same Input Id
			AbilitySystemComponent->FindAbilitySpecFromClass(TSubclassOf<UGameplayAbility>(AbilityBindingData.AbilityClass.LoadSynchronous()));
		}
		else
		{
			UE_LOG(LogGameplayFeaturesExtraActions_Internal, Error, TEXT("%s: Failed to find AbilitySystemComponent on Actor %s."),
			       *FString(__FUNCTION__), *TargetActor->GetName());
		}
	}
	// If we are not using an existing spec, we'll create a basic spec just to pass some parameters to the ability binding
	else
	{
		// Only add the class if it's valid
		if (!AbilityBindingData.AbilityClass.IsNull())
		{
			NewAbilitySpec.Ability = Cast<UGameplayAbility>(AbilityBindingData.AbilityClass.LoadSynchronous()->GetDefaultObject());
		}

		// Only append tags if the container is not empty
		if (!AbilityBindingData.AbilityTags.IsEmpty())
		{
			NewAbilitySpec.Ability->AbilityTags.AppendTags(AbilityBindingData.AbilityTags);
		}

		NewAbilitySpec.InputID = InputID;
	}

	return NewAbilitySpec;
}
