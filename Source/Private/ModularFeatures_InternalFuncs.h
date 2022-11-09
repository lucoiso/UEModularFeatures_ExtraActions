// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilityInputBinding.h"
#include "GameFeatureAction_WorldActionBase.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Controller.h"
#include "ModularFeatures_ExtraActions.h"
#include "MFEA_Settings.h"

namespace ModularFeaturesHelper
{
	const UMFEA_Settings* GetPluginSettings()
	{
		static const UMFEA_Settings* Instance = GetDefault<UMFEA_Settings>();
		return Instance;
	}

	bool ActorHasAllRequiredTags(const AActor* Actor, const TArray<FName>& RequiredTags)
	{
		if (!IsValid(Actor))
		{
			return false;
		}

		for (const FName& Tag : RequiredTags)
		{
			if (!Actor->ActorHasTag(Tag))
			{
				return false;
			}
		}

		return true;
	}

	UAbilitySystemComponent* GetAbilitySystemComponentByActor(AActor* InActor)
	{
		const IAbilitySystemInterface* const InterfaceOwner = Cast<IAbilitySystemInterface>(InActor);
		return InterfaceOwner != nullptr ? InterfaceOwner->GetAbilitySystemComponent() : InActor->FindComponentByClass<UAbilitySystemComponent>();
	}

	EInputBindingOwner GetValidatedInputBindingOwner(const EInputBindingOwnerOverride& InOwner)
	{
		switch (InOwner)
		{
		case EInputBindingOwnerOverride::Pawn: return EInputBindingOwner::Pawn;
		case EInputBindingOwnerOverride::Controller: return EInputBindingOwner::Controller;
		default: break;
		}

		return GetPluginSettings()->InputBindingOwner;
	}

	IAbilityInputBinding* GetAbilityInputBindingInterface(AActor* InActor, const EInputBindingOwnerOverride& InOwner)
	{
		if (!IsValid(InActor))
		{
			return nullptr;
		}

		if (APawn* const TargetPawn = Cast<APawn>(InActor))
		{
			switch (GetValidatedInputBindingOwner(InOwner))
			{
				case EInputBindingOwner::Pawn: return Cast<IAbilityInputBinding>(TargetPawn);

				case EInputBindingOwner::Controller: return Cast<IAbilityInputBinding>(TargetPawn->GetController());

				default: return nullptr;
			}
		}

		return nullptr;
	}

	UEnhancedInputComponent* GetEnhancedInputComponentInPawn(APawn* InPawn)
	{
		if (!IsValid(InPawn))
		{
			return nullptr;
		}

		return Cast<UEnhancedInputComponent>(InPawn->GetController()->InputComponent.Get());
	}

	AActor* GetPawnInputOwner(APawn* InPawn, const EInputBindingOwnerOverride& InOwner)
	{
		if (!IsValid(InPawn))
		{
			return nullptr;
		}

		switch (GetValidatedInputBindingOwner(InOwner))
		{
			case EInputBindingOwner::Pawn: return Cast<AActor>(InPawn);
			case EInputBindingOwner::Controller: return Cast<AActor>(InPawn->GetController());
			default: break;
		}

		return nullptr;
	}

	UEnum* LoadInputEnum()
	{
		if (GetPluginSettings()->InputIDEnumeration.IsNull())
		{
			UE_LOG(LogGameplayFeaturesExtraActions, Error, TEXT("%s: bUseInputEnumeration is set to true but Enumeration class is null!"), *FString(__func__));
			return nullptr;
		}

		return GetPluginSettings()->InputIDEnumeration.LoadSynchronous();
	}

	// Will be removed in the future - but i don't want to break existing projects :)
	const bool BindAbilityInputToInterfaceOwnerWithID(const IAbilityInputBinding* TargetInterfaceOwner, UInputAction* InputAction, const int32 InputID)
	{
		if (!TargetInterfaceOwner)
		{
			UE_LOG(LogGameplayFeaturesExtraActions, Error, TEXT("%s: Failed to setup input binding on Actor %s due to a invalid interface owner."), *FString(__func__), *TargetInterfaceOwner->_getUObject()->GetName());

			return false;
		}

		IAbilityInputBinding::Execute_SetupAbilityInputBinding(TargetInterfaceOwner->_getUObject(), InputAction, InputID);

		return true;
	}
	
	const bool BindAbilityInputToInterfaceOwner(const IAbilityInputBinding* TargetInterfaceOwner, UInputAction* InputAction, FGameplayAbilitySpec& AbilitySpec)
	{
		if (!BindAbilityInputToInterfaceOwnerWithID(TargetInterfaceOwner, InputAction, AbilitySpec.InputID))
		{
			return false;
		}

		switch (GetPluginSettings()->AbilityBindingMode)
		{
			case (EAbilityBindingMode::InputID):
				IAbilityInputBinding::Execute_SetupAbilityBindingByInput(TargetInterfaceOwner->_getUObject(), InputAction, AbilitySpec.InputID);
				break;

			case (EAbilityBindingMode::AbilitySpec):
				IAbilityInputBinding::Execute_SetupAbilityBindingBySpec(TargetInterfaceOwner->_getUObject(), InputAction, AbilitySpec);
				break;

			case (EAbilityBindingMode::AbilityTags):
				IAbilityInputBinding::Execute_SetupAbilityBindingByTags(TargetInterfaceOwner->_getUObject(), InputAction, AbilitySpec.Ability->AbilityTags);
				break;

			case (EAbilityBindingMode::AbilityClass):
				IAbilityInputBinding::Execute_SetupAbilityBindingByClass(TargetInterfaceOwner->_getUObject(), InputAction, TSubclassOf<UGameplayAbility>(AbilitySpec.Ability->GetClass()));
				break;

			default:
				return false;
		}

		return true;
	}

	void RemoveAbilityInputInInterfaceOwner(UObject* InterfaceOwner, TArray<TWeakObjectPtr<UInputAction>>& ActionArr)
	{
		if (!IsValid(InterfaceOwner))
		{
			UE_LOG(LogGameplayFeaturesExtraActions, Error, TEXT("%s: Failed to remove input binding due to invalid interface owner."), *FString(__func__));			
			return;
		}

		for (TWeakObjectPtr<UInputAction>& InputRef : ActionArr)
		{
			if (InputRef.IsValid())
			{
				IAbilityInputBinding::Execute_RemoveAbilityInputBinding(InterfaceOwner, InputRef.Get());
			}

			InputRef.Reset();
		}

		ActionArr.Empty();
	}

	const bool IsUsingInputIDEnumeration()
	{
		return GetPluginSettings()->AbilityBindingMode == EAbilityBindingMode::InputID;
	}

	const int32 GetInputIDByName(const FName DisplayName, UEnum* Enumeration = nullptr)
	{
		if (!IsUsingInputIDEnumeration())
		{
			return -1;
		}

		if (!Enumeration)
		{
			Enumeration = GetPluginSettings()->InputIDEnumeration.LoadSynchronous();
		}

		return Enumeration->GetValueByName(DisplayName, EGetByNameFlags::CheckAuthoredName);
	}
}
