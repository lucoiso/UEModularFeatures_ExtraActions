// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#include "Actions/GameFeatureAction_AddEffects.h"
#include "ModularFeatures_InternalFuncs.h"
#include <Engine/GameInstance.h>
#include <Components/GameFrameworkComponentManager.h>

#ifdef UE_INLINE_GENERATED_CPP_BY_NAME
#include UE_INLINE_GENERATED_CPP_BY_NAME(GameFeatureAction_AddEffects)
#endif

void UGameFeatureAction_AddEffects::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	if (!ensureAlways(ActiveExtensions.IsEmpty()))
	{
		ResetExtension();
	}

	Super::OnGameFeatureActivating(Context);
}

void UGameFeatureAction_AddEffects::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);
	ResetExtension();
}

void UGameFeatureAction_AddEffects::ResetExtension()
{
	while (!ActiveExtensions.IsEmpty())
	{
		const auto ExtensionIterator = ActiveExtensions.CreateConstIterator();
		RemoveEffects(ExtensionIterator->Key.Get());
	}

	Super::ResetExtension();
}

void UGameFeatureAction_AddEffects::AddToWorld(const FWorldContext& WorldContext)
{
	if (UGameFrameworkComponentManager* const ComponentManager = GetGameFrameworkComponentManager(WorldContext); IsValid(ComponentManager) && !
		TargetPawnClass.IsNull())
	{
		using FHandlerDelegate = UGameFrameworkComponentManager::FExtensionHandlerDelegate;
		const FHandlerDelegate ExtensionHandlerDelegate = FHandlerDelegate::CreateUObject(this, &UGameFeatureAction_AddEffects::HandleActorExtension);

		ActiveRequests.Add(ComponentManager->AddExtensionHandler(TargetPawnClass, ExtensionHandlerDelegate));
	}
}

void UGameFeatureAction_AddEffects::HandleActorExtension(AActor* Owner, const FName EventName)
{
	if (EventName == UGameFrameworkComponentManager::NAME_ExtensionRemoved || EventName == UGameFrameworkComponentManager::NAME_ReceiverRemoved)
	{
		RemoveEffects(Owner);
	}

	else if (EventName == UGameFrameworkComponentManager::NAME_ExtensionAdded || EventName == UGameFrameworkComponentManager::NAME_GameActorReady)
	{
		// We don't want to repeat the addition and cannot add if the user don't have the required tags
		if (ActiveExtensions.Contains(Owner) || !ModularFeaturesHelper::ActorHasAllRequiredTags(Owner, RequireTags))
		{
			return;
		}

		for (const FEffectStackedData& Entry : Effects)
		{
			if (Entry.EffectClass.IsNull())
			{
				UE_LOG(LogGameplayFeaturesExtraActions_Internal, Error, TEXT("%s: Effect class is null."), *FString(__FUNCTION__));
			}
			else
			{
				AddEffects(Owner, Entry);
			}
		}
	}
}

void UGameFeatureAction_AddEffects::AddEffects(AActor* TargetActor, const FEffectStackedData& Effect)
{
	// Only proceed if the target actor is valid and has authority
	if (!IsValid(TargetActor) || TargetActor->GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	// Get the ability system component of the target actor
	if (UAbilitySystemComponent* const AbilitySystemComponent = ModularFeaturesHelper::GetAbilitySystemComponentInActor(TargetActor))
	{
		// Check if there's already added spec data applied to the target actor
		TArray<FActiveGameplayEffectHandle>& SpecData = ActiveExtensions.FindOrAdd(TargetActor);

		// Load the Effect class into a const variable
		const TSubclassOf<UGameplayEffect> EffectClass = Effect.EffectClass.LoadSynchronous();

		UE_LOG(LogGameplayFeaturesExtraActions_Internal, Display, TEXT("%s: Adding effect %s level %u to Actor %s with %u SetByCaller params."),
		       *FString(__FUNCTION__), *EffectClass->GetName(), Effect.EffectLevel, *TargetActor->GetName(), Effect.SetByCallerParams.Num());

		// Create the outgoing spec handle to define the SetByCaller settings and apply the data to the target Ability System Component
		const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
			EffectClass, Effect.EffectLevel, AbilitySystemComponent->MakeEffectContext());

		// Iterate the Set By Caller params and add the data to the Spec Handle Data
		for (const TPair<FGameplayTag, float>& SetByCallerParam : Effect.SetByCallerParams)
		{
			SpecHandle.Data.Get()->SetSetByCallerMagnitude(SetByCallerParam.Key, SetByCallerParam.Value);
		}

		// Apply the effect data to the target Ability System Component
		const FActiveGameplayEffectHandle NewActiveEffect = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

		// Add the active effect handle to the Spec Data before adding it to the ActiveExtensions map
		SpecData.Add(NewActiveEffect);

		ActiveExtensions.Add(TargetActor, SpecData);
	}
	else
	{
		UE_LOG(LogGameplayFeaturesExtraActions_Internal, Error, TEXT("%s: Failed to find AbilitySystemComponent on Actor %s."), *FString(__FUNCTION__),
		       *TargetActor->GetName());
	}
}

void UGameFeatureAction_AddEffects::RemoveEffects(AActor* TargetActor)
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

	// Get the active effects and check if it's empty
	if (TArray<FActiveGameplayEffectHandle> ActiveEffects = ActiveExtensions.FindRef(TargetActor); !ActiveEffects.IsEmpty())
	{
		// Get the target ability system component
		if (UAbilitySystemComponent* const AbilitySystemComponent = ModularFeaturesHelper::GetAbilitySystemComponentInActor(TargetActor))
		{
			UE_LOG(LogGameplayFeaturesExtraActions_Internal, Display, TEXT("%s: Removing effects from Actor %s."), *FString(__FUNCTION__),
			       *TargetActor->GetName());

			// Iterate through the active effects and remove the specified effect by its effect handle if its valid
			for (const FActiveGameplayEffectHandle& EffectHandle : ActiveEffects)
			{
				if (!EffectHandle.IsValid())
				{
					continue;
				}

				AbilitySystemComponent->RemoveActiveGameplayEffect(EffectHandle);
			}
		}
		else if (IsValid(GetWorld()) && IsValid(GetWorld()->GetGameInstance()))
		{
			UE_LOG(LogGameplayFeaturesExtraActions_Internal, Error, TEXT("%s: Failed to find AbilitySystemComponent on Actor %s."),
			       *FString(__FUNCTION__), *TargetActor->GetName());
		}
	}

	ActiveExtensions.Remove(TargetActor);
}
