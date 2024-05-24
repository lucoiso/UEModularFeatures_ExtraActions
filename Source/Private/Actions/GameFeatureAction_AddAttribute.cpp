// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#include "Actions/GameFeatureAction_AddAttribute.h"
#include "ModularFeatures_InternalFuncs.h"
#include <Components/GameFrameworkComponentManager.h>
#include <Engine/GameInstance.h>
#include <Runtime/Launch/Resources/Version.h>

#ifdef UE_INLINE_GENERATED_CPP_BY_NAME
#include UE_INLINE_GENERATED_CPP_BY_NAME(GameFeatureAction_AddAttribute)
#endif

void UGameFeatureAction_AddAttribute::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	if (!ensureAlways(ActiveExtensions.IsEmpty()))
	{
		ResetExtension();
	}

	Super::OnGameFeatureActivating(Context);
}

void UGameFeatureAction_AddAttribute::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);
	ResetExtension();
}

void UGameFeatureAction_AddAttribute::ResetExtension()
{
	while (!ActiveExtensions.IsEmpty())
	{
		const auto ExtensionIterator = ActiveExtensions.CreateConstIterator();
		RemoveAttribute(ExtensionIterator->Key.Get());
	}

	Super::ResetExtension();
}

void UGameFeatureAction_AddAttribute::AddToWorld(const FWorldContext& WorldContext)
{
	if (UGameFrameworkComponentManager* const ComponentManager = GetGameFrameworkComponentManager(WorldContext); IsValid(ComponentManager) && !
		TargetPawnClass.IsNull())
	{
		using FHandlerDelegate = UGameFrameworkComponentManager::FExtensionHandlerDelegate;
		const FHandlerDelegate ExtensionHandlerDelegate = FHandlerDelegate::CreateUObject(
			this, &UGameFeatureAction_AddAttribute::HandleActorExtension);

		ActiveRequests.Add(ComponentManager->AddExtensionHandler(TargetPawnClass, ExtensionHandlerDelegate));
	}
}

void UGameFeatureAction_AddAttribute::HandleActorExtension(AActor* Owner, const FName EventName)
{
	if (EventName == UGameFrameworkComponentManager::NAME_ExtensionRemoved || EventName == UGameFrameworkComponentManager::NAME_ReceiverRemoved)
	{
		RemoveAttribute(Owner);
	}

	else if (EventName == UGameFrameworkComponentManager::NAME_ExtensionAdded || EventName == UGameFrameworkComponentManager::NAME_GameActorReady)
	{
		// We don't want to repeat the addition and cannot add if the user don't have the required tags
		if (ActiveExtensions.Contains(Owner) || !ModularFeaturesHelper::ActorHasAllRequiredTags(Owner, RequireTags))
		{
			return;
		}

		if (Attribute.IsNull())
		{
			UE_LOG(LogGameplayFeaturesExtraActions_Internal, Error, TEXT("%s: Attribute is null."), *FString(__FUNCTION__));
		}
		else
		{
			AddAttribute(Owner);
		}
	}
}

void UGameFeatureAction_AddAttribute::AddAttribute(AActor* TargetActor)
{
	// Only proceed if the target actor is valid and has authority
	if (!IsValid(TargetActor) || TargetActor->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	// Get the ability system component of the target actor
	if (UAbilitySystemComponent* const AbilitySystemComponent = ModularFeaturesHelper::GetAbilitySystemComponentInActor(TargetActor))
	{
		// Load and store the AttributeSet Class into a const variable
		if (const TSubclassOf<UAttributeSet> SetType = Attribute.LoadSynchronous())
		{
			// Create the AttributeSet object
			UAttributeSet* const NewSet = NewObject<UAttributeSet>(AbilitySystemComponent->GetOwnerActor(), SetType);

			// Check if the user wants to initialize the AttributeSet with a DataTable. If true, will load and initialize the AttributeSet using the metadatas from it
			if (!InitializationData.IsNull())
			{
				NewSet->InitFromMetaDataTable(InitializationData.LoadSynchronous());
			}

			// Add the attribute set to the ability system component
			AbilitySystemComponent->AddAttributeSetSubobject(NewSet);

			// Force the ability system component to replicate the attribute addition
			AbilitySystemComponent->ForceReplication();

			UE_LOG(LogGameplayFeaturesExtraActions_Internal, Display, TEXT("%s: Attribute %s added to Actor %s."), *FString(__FUNCTION__),
			       *SetType->GetName(), *TargetActor->GetName());

			ActiveExtensions.Add(TargetActor, NewSet);
		}
		else
		{
			UE_LOG(LogGameplayFeaturesExtraActions_Internal, Error, TEXT("%s: Attribute is invalid."), *FString(__FUNCTION__));
		}
	}
	else
	{
		UE_LOG(LogGameplayFeaturesExtraActions_Internal, Error, TEXT("%s: Failed to find AbilitySystemComponent on Actor %s."), *FString(__FUNCTION__),
		       *TargetActor->GetName());
	}
}

void UGameFeatureAction_AddAttribute::RemoveAttribute(AActor* TargetActor)
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

	// Get the ability system component of the target actor
	if (UAbilitySystemComponent* const AbilitySystemComponent = ModularFeaturesHelper::GetAbilitySystemComponentInActor(TargetActor))
	{
		// Get the added Attribute Set to the target actor by searching inside the Active Extensions, remove it from the Ability System Component and force a replication
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION == 0
        if (UAttributeSet* const AttributeToRemove = ActiveExtensions.FindRef(TargetActor).Get(); AbilitySystemComponent->GetSpawnedAttributes_Mutable().Remove(AttributeToRemove) != 0)
        {
            UE_LOG(LogGameplayFeaturesExtraActions_Internal, Display, TEXT("%s: Attribute %s removed from Actor %s."), *FString(__FUNCTION__), *AttributeToRemove->GetName(), *TargetActor->GetName());
            AbilitySystemComponent->ForceReplication();
        }
#else
		if (UAttributeSet* const AttributeToRemove = ActiveExtensions.FindRef(TargetActor).Get())
		{
			UE_LOG(LogGameplayFeaturesExtraActions_Internal, Display, TEXT("%s: Removing attribute %s from Actor %s."), *FString(__FUNCTION__),
			       *AttributeToRemove->GetName(), *TargetActor->GetName());

			AbilitySystemComponent->RemoveSpawnedAttribute(AttributeToRemove);
			AbilitySystemComponent->ForceReplication();
		}
#endif
	}
	// We don't need to warn the user if there's no valid world or game instance, since this is a common case when the game is shutting down
	else if (IsValid(GetWorld()) && IsValid(GetWorld()->GetGameInstance()))
	{
		UE_LOG(LogGameplayFeaturesExtraActions_Internal, Error, TEXT("%s: Failed to find AbilitySystemComponent on Actor %s."), *FString(__FUNCTION__),
		       *TargetActor->GetName());
	}

	ActiveExtensions.Remove(TargetActor);
}
