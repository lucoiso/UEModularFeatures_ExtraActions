// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#include "Actions/GameFeatureAction_AddAbilities.h"
#include "ModularFeatures_InternalFuncs.h"
#include <Components/GameFrameworkComponentManager.h>
#include <InputAction.h>

void UGameFeatureAction_AddAbilities::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	if (!ensureAlways(ActiveExtensions.IsEmpty()) || !ensureAlways(ActiveRequests.IsEmpty()))
	{
		ResetExtension();
	}

	Super::OnGameFeatureActivating(Context);
}

void UGameFeatureAction_AddAbilities::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);

	ResetExtension();
}

void UGameFeatureAction_AddAbilities::ResetExtension()
{
	while (!ActiveExtensions.IsEmpty())
	{
		const auto ExtensionIterator = ActiveExtensions.CreateConstIterator();
		RemoveActorAbilities(ExtensionIterator->Key.Get());
	}

	ActiveRequests.Empty();
}

void UGameFeatureAction_AddAbilities::AddToWorld(const FWorldContext& WorldContext)
{
	if (UGameFrameworkComponentManager* const ComponentManager = GetGameFrameworkComponentManager(WorldContext);
		IsValid(ComponentManager) && !TargetPawnClass.IsNull())
	{
		using FHandlerDelegate = UGameFrameworkComponentManager::FExtensionHandlerDelegate;

		const FHandlerDelegate ExtensionHandlerDelegate = FHandlerDelegate::CreateUObject(this, &UGameFeatureAction_AddAbilities::HandleActorExtension);

		ActiveRequests.Add(ComponentManager->AddExtensionHandler(TargetPawnClass, ExtensionHandlerDelegate));
	}
}

void UGameFeatureAction_AddAbilities::HandleActorExtension(AActor* Owner, const FName EventName)
{
	UE_LOG(LogGameplayFeaturesExtraActions, Display, TEXT("Event %s sent by Actor %s for ability management."), *EventName.ToString(), *Owner->GetName());

	if (EventName == UGameFrameworkComponentManager::NAME_ExtensionRemoved || EventName == UGameFrameworkComponentManager::NAME_ReceiverRemoved)
	{
		RemoveActorAbilities(Owner);
	}

	else if (EventName == UGameFrameworkComponentManager::NAME_ExtensionAdded || EventName == UGameFrameworkComponentManager::NAME_GameActorReady)
	{
		// We don't want to repeat the addition and cannot add if the user don't have the required tags
		if (ActiveExtensions.Contains(Owner) || !ModularFeaturesHelper::ActorHasAllRequiredTags(Owner, RequireTags))
		{
			return;
		}

		InputIDEnumeration_Ptr.Reset();

		// Load the enumeration and save it before the loop to avoid high disk consumption due to loading a soft reference a lot of times since there's only 1 enumeration
		InputIDEnumeration_Ptr = ModularFeaturesHelper::LoadInputEnum();
			
		for (const FAbilityMapping& Entry : Abilities)
		{
			if (Entry.AbilityClass.IsNull())
			{
				UE_LOG(LogGameplayFeaturesExtraActions, Error, TEXT("%s: Ability class is null."), *FString(__func__));
			}
			else
			{
				AddActorAbilities(Owner, Entry);
			}
		}

		// Free the pointer. We only need this enum when we're adding abilities
		InputIDEnumeration_Ptr.Reset();
	}
}

void UGameFeatureAction_AddAbilities::AddActorAbilities(AActor* TargetActor, const FAbilityMapping& Ability)
{
	// Only proceed if the target actor is valid and has authority
	if (!IsValid(TargetActor) || TargetActor->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (UAbilitySystemComponent* const AbilitySystemComponent = ModularFeaturesHelper::GetAbilitySystemComponentByActor(TargetActor))
	{
		// If InputID Enumeration using is disabled, assume -1 as value
		const int32 InputID = ModularFeaturesHelper::GetInputIDByName(Ability.InputIDValueName, InputIDEnumeration_Ptr.Get());
		
		//Check if there's a existing ability data already loaded
		FActiveAbilityData& NewAbilityData = ActiveExtensions.FindOrAdd(TargetActor);

		// Load the ability class and store into a const variable
		const TSubclassOf<UGameplayAbility> AbilityToAdd = Ability.AbilityClass.LoadSynchronous();
		
		UE_LOG(LogGameplayFeaturesExtraActions, Display, TEXT("%s: Adding ability %s to Actor %s."), *FString(__func__), *AbilityToAdd->GetName(), *TargetActor->GetName());
		
		// Create the spec, used to give the ability to target's ability system component
		FGameplayAbilitySpec NewAbilitySpec(AbilityToAdd, Ability.AbilityLevel, InputID, TargetActor);

		// Try to give the ability to the target and check if the spec handle is valid
		if (const FGameplayAbilitySpecHandle NewSpecHandle = AbilitySystemComponent->GiveAbility(NewAbilitySpec);
			NewSpecHandle.IsValid())
		{
			// Add the spec handle to the ability data
			NewAbilityData.SpecHandle.Add(NewSpecHandle);
						
			// Only bind the input if the Input Action is valid. This is not mandatory due to passive abilities that don't need to be associated to inputs
			if (!Ability.InputAction.IsNull())
			{
				// Get the target Ability Input Binding interface
				const IMFEA_AbilityInputBinding* const SetupInputInterface = ModularFeaturesHelper::GetAbilityInputBindingInterface(TargetActor, InputBindingOwnerOverride);

				// If we can bind the input to the target interface, we must add the input reference to the ability data
				if (UInputAction* const AbilityInput = Ability.InputAction.LoadSynchronous(); 
					ModularFeaturesHelper::BindAbilityInputToInterfaceOwner(SetupInputInterface, AbilityInput, NewAbilitySpec))
				{
					NewAbilityData.InputReference.Add(AbilityInput);
				}
			}
			
			ActiveExtensions.Add(TargetActor, NewAbilityData);
		}
	}
	else
	{
		UE_LOG(LogGameplayFeaturesExtraActions, Error, TEXT("%s: Failed to find AbilitySystemComponent on Actor %s."), *FString(__func__), *TargetActor->GetName());
	}
}

void UGameFeatureAction_AddAbilities::RemoveActorAbilities(AActor* TargetActor)
{
	// Only proceed if the target actor is valid
	if (!IsValid(TargetActor))
	{
		ActiveExtensions.Remove(TargetActor);
		return;
	}

	// Only proceed if the target actor has authority
	if (TargetActor->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	// Check if we can remove the abilities from this target actor by checking if it is inside the active extensions map
	FActiveAbilityData ActiveAbilities = ActiveExtensions.FindRef(TargetActor);
	if constexpr (&ActiveAbilities == nullptr)
	{
		UE_LOG(LogGameplayFeaturesExtraActions, Warning, TEXT("%s: No active abilities found for Actor %s."), *FString(__func__), *TargetActor->GetName());
		ActiveExtensions.Remove(TargetActor);
		return;
	}
	
	if (UAbilitySystemComponent* const AbilitySystemComponent = ModularFeaturesHelper::GetAbilitySystemComponentByActor(TargetActor))
	{
		UE_LOG(LogGameplayFeaturesExtraActions, Display, TEXT("%s: Removing associated abilities from Actor %s."), *FString(__func__), *TargetActor->GetName());

		// Iterate the active abilities and remove all spec handle associated to this actor
		for (const FGameplayAbilitySpecHandle& SpecHandle : ActiveAbilities.SpecHandle)
		{
			if (SpecHandle.IsValid())
			{
				// Set the ability to be removed on end and clear it
				AbilitySystemComponent->SetRemoveAbilityOnEnd(SpecHandle);
				AbilitySystemComponent->ClearAbility(SpecHandle);
			}
		}

		// Get the interface owner and try to remove the input bindings
		if (const IMFEA_AbilityInputBinding* const SetupInputInterface = ModularFeaturesHelper::GetAbilityInputBindingInterface(TargetActor, InputBindingOwnerOverride))
		{
			ModularFeaturesHelper::RemoveAbilityInputInInterfaceOwner(SetupInputInterface->_getUObject(), ActiveAbilities.InputReference);
		}
	}
	else if (IsValid(GetWorld()) && IsValid(GetWorld()->GetGameInstance()))
	{
		UE_LOG(LogGameplayFeaturesExtraActions, Error, TEXT("%s: Failed to find AbilitySystemComponent on Actor %s."), *FString(__func__), *TargetActor->GetName());
	}

	ActiveExtensions.Remove(TargetActor);
}