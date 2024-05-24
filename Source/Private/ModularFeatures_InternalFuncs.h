// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#pragma once

#include <CoreMinimal.h>
#include <AbilitySystemComponent.h>
#include <AbilitySystemGlobals.h>
#include <EnhancedInputComponent.h>
#include <GameFramework/Controller.h>
#include <GameFramework/Pawn.h>
#include "Interfaces/MFEA_AbilityInputBinding.h"
#include "LogModularFeatures_ExtraActions.h"
#include "MFEA_Settings.h"

namespace ModularFeaturesHelper
{
	static const UMFEA_Settings* GetPluginSettings()
	{
		static const UMFEA_Settings* Instance = GetDefault<UMFEA_Settings>();
		return Instance;
	}

	static bool ActorHasAllRequiredTags(const AActor* Actor, const TArray<FName>& RequiredTags)
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

	static UAbilitySystemComponent* GetAbilitySystemComponentInActor(AActor* InActor)
	{
		return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InActor);
	}

	static EInputBindingOwner GetValidatedInputBindingOwner(const EInputBindingOwnerOverride& InOwner)
	{
		switch (InOwner)
		{
		case EInputBindingOwnerOverride::Pawn: return EInputBindingOwner::Pawn;
		case EInputBindingOwnerOverride::Controller: return EInputBindingOwner::Controller;
		default: break;
		}

		return GetPluginSettings()->InputBindingOwner;
	}

	static IMFEA_AbilityInputBinding* GetAbilityInputBindingInterface(AActor* InActor, const EInputBindingOwnerOverride& InOwner)
	{
		if (!IsValid(InActor))
		{
			return nullptr;
		}

		if (APawn* const TargetPawn = Cast<APawn>(InActor))
		{
			switch (GetValidatedInputBindingOwner(InOwner))
			{
			case EInputBindingOwner::Pawn: return Cast<IMFEA_AbilityInputBinding>(TargetPawn);
			case EInputBindingOwner::Controller: return Cast<IMFEA_AbilityInputBinding>(TargetPawn->GetController());
			default: return nullptr;
			}
		}

		return nullptr;
	}

	static UEnhancedInputComponent* GetEnhancedInputComponentInPawn(APawn* InPawn)
	{
		if (!IsValid(InPawn))
		{
			return nullptr;
		}

		return Cast<UEnhancedInputComponent>(InPawn->GetController()->InputComponent.Get());
	}

	static AActor* GetPawnInputOwner(APawn* InPawn, const EInputBindingOwnerOverride& InOwner)
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

	static UEnum* LoadInputEnum()
	{
		if (GetPluginSettings()->InputIDEnumeration.IsNull())
		{
			UE_LOG(LogGameplayFeaturesExtraActions_Internal, Error, TEXT("%s: bUseInputEnumeration is set to true but Enumeration class is null!"),
			       *FString(__FUNCTION__));
			return nullptr;
		}

		return GetPluginSettings()->InputIDEnumeration.LoadSynchronous();
	}

	// Will be removed in the future - but i don't want to break existing projects :)
	static const bool BindAbilityInputToInterfaceOwnerWithID(const IMFEA_AbilityInputBinding* TargetInterfaceOwner, UInputAction* InputAction,
	                                                         const int32 InputID)
	{
		if (!TargetInterfaceOwner)
		{
			UE_LOG(LogGameplayFeaturesExtraActions_Internal, Error,
			       TEXT("%s: Failed to setup input binding on Actor %s due to a invalid interface owner."), *FString(__FUNCTION__),
			       *TargetInterfaceOwner->_getUObject()->GetName());

			return false;
		}

		return true;
	}

	static const bool BindAbilityInputToInterfaceOwner(const IMFEA_AbilityInputBinding* TargetInterfaceOwner, UInputAction* InputAction,
	                                                   const FGameplayAbilitySpec& AbilitySpec)
	{
		if (!BindAbilityInputToInterfaceOwnerWithID(TargetInterfaceOwner, InputAction, AbilitySpec.InputID))
		{
			return false;
		}

		switch (GetPluginSettings()->AbilityBindingMode)
		{
		case (EAbilityBindingMode::InputID):
			IMFEA_AbilityInputBinding::Execute_SetupAbilityBindingByInput(TargetInterfaceOwner->_getUObject(), InputAction, AbilitySpec.InputID);
			break;

		case (EAbilityBindingMode::AbilitySpec):
			IMFEA_AbilityInputBinding::Execute_SetupAbilityBindingBySpec(TargetInterfaceOwner->_getUObject(), InputAction, AbilitySpec);
			break;

		case (EAbilityBindingMode::AbilityTags):
			IMFEA_AbilityInputBinding::Execute_SetupAbilityBindingByTags(TargetInterfaceOwner->_getUObject(), InputAction,
			                                                             AbilitySpec.Ability->AbilityTags);
			break;

		case (EAbilityBindingMode::AbilityClass):
			IMFEA_AbilityInputBinding::Execute_SetupAbilityBindingByClass(TargetInterfaceOwner->_getUObject(), InputAction,
			                                                              TSubclassOf<UGameplayAbility>(AbilitySpec.Ability->GetClass()));
			break;

		default:
			return false;
		}

		return true;
	}

	static void RemoveAbilityInputInInterfaceOwner(UObject* InterfaceOwner, TArray<TWeakObjectPtr<UInputAction>>& ActionArr)
	{
		if (!IsValid(InterfaceOwner))
		{
			UE_LOG(LogGameplayFeaturesExtraActions_Internal, Error, TEXT("%s: Failed to remove input binding due to invalid interface owner."),
			       *FString(__FUNCTION__));
			return;
		}

		for (TWeakObjectPtr<UInputAction>& InputRef : ActionArr)
		{
			if (InputRef.IsValid())
			{
				IMFEA_AbilityInputBinding::Execute_RemoveAbilityInputBinding(InterfaceOwner, InputRef.Get());
			}

			InputRef.Reset();
		}

		ActionArr.Empty();
	}

	static const bool IsUsingInputIDEnumeration()
	{
		return GetPluginSettings()->AbilityBindingMode == EAbilityBindingMode::InputID;
	}

	static const int32 GetInputIDByName(const FName DisplayName, UEnum* Enumeration = nullptr)
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
