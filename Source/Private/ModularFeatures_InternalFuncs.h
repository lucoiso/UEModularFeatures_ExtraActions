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

	static UAbilitySystemComponent* GetAbilitySystemComponentByActor(AActor* InActor)
	{
		const IAbilitySystemInterface* const InterfaceOwner = Cast<IAbilitySystemInterface>(InActor);
		return InterfaceOwner != nullptr ? InterfaceOwner->GetAbilitySystemComponent() : InActor->FindComponentByClass<UAbilitySystemComponent>();
	}

	static IAbilityInputBinding* GetAbilityInputBindingInterface(AActor* InActor, const EControllerOwner& InOwner)
	{
		if (!IsValid(InActor))
		{
			return nullptr;
		}

		if (APawn* const TargetPawn = Cast<APawn>(InActor))
		{
			switch (InOwner)
			{
				case EControllerOwner::Pawn: return Cast<IAbilityInputBinding>(TargetPawn);

				case EControllerOwner::Controller: return Cast<IAbilityInputBinding>(TargetPawn->GetController());

				default: return nullptr;
			}
		}

		return nullptr;
	}

	static UEnhancedInputComponent* GetEnhancedInputComponentInPawn(APawn* InPawn, const EControllerOwner& InOwner)
	{
		if (!IsValid(InPawn))
		{
			return nullptr;
		}

		switch (InOwner)
		{
			case EControllerOwner::Pawn: return Cast<UEnhancedInputComponent>(InPawn->InputComponent.Get());

			case EControllerOwner::Controller: return Cast<UEnhancedInputComponent>(InPawn->GetController()->InputComponent.Get());

			default: break;
		}

		return nullptr;
	}

	static AActor* GetPawnInputOwner(APawn* InPawn, const EControllerOwner& InOwner)
	{
		if (!IsValid(InPawn))
		{
			return nullptr;
		}

		switch (InOwner)
		{
			case EControllerOwner::Pawn: return Cast<AActor>(InPawn);

			case EControllerOwner::Controller: return Cast<AActor>(InPawn->GetController());

			default: break;
		}

		return nullptr;
	}

	static UEnum* LoadInputEnum()
	{
		if (GetPluginSettings()->InputIDEnumeration.IsNull())
		{
			UE_LOG(LogGameplayFeaturesExtraActions, Error, TEXT("%s: bUseInputEnumeration is set to true but Enumeration class is null!"), *FString(__func__));
			return nullptr;
		}

		return GetPluginSettings()->InputIDEnumeration.LoadSynchronous();
	}

	static const bool BindAbilityInputToInterfaceOwner(const IAbilityInputBinding* TargetInterfaceOwner, UInputAction* InputAction, const int32 InputID)
	{
		if (!TargetInterfaceOwner)
		{
			UE_LOG(LogGameplayFeaturesExtraActions, Error, TEXT("%s: Failed to setup input binding on Actor %s due to a invalid interface owner."), *FString(__func__), *TargetInterfaceOwner->_getUObject()->GetName());

			return false;
		}

		IAbilityInputBinding::Execute_SetupAbilityInputBinding(TargetInterfaceOwner->_getUObject(), InputAction, InputID);

		return true;
	}

	static void RemoveAbilityInputInInterfaceOwner(UObject* InterfaceOwner, TArray<TWeakObjectPtr<UInputAction>>& ActionArr)
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

	static const int32 GetInputIDByName(const FName DisplayName, UEnum* Enumeration = nullptr)
	{
		if (!GetPluginSettings()->bUseInputEnumeration)
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
