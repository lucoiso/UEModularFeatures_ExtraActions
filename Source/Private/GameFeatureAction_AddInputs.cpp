// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#include "GameFeatureAction_AddInputs.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "AbilityInputBinding.h"
#include "Components/GameFrameworkComponentManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "ModularFeatures_InternalFuncs.h"

void UGameFeatureAction_AddInputs::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	if (!ensureAlways(ActiveExtensions.IsEmpty()) || !ensureAlways(ActiveRequests.IsEmpty()))
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

	ActiveRequests.Empty();
}

void UGameFeatureAction_AddInputs::AddToWorld(const FWorldContext& WorldContext)
{
	if (UGameFrameworkComponentManager* const ComponentManager = GetGameFrameworkComponentManager(WorldContext);
		IsValid(ComponentManager) && !TargetPawnClass.IsNull())
	{
		using FHandlerDelegate = UGameFrameworkComponentManager::FExtensionHandlerDelegate;

		const FHandlerDelegate ExtensionHandlerDelegate = FHandlerDelegate::CreateUObject(this, &UGameFeatureAction_AddInputs::HandleActorExtension);

		ActiveRequests.Add(ComponentManager->AddExtensionHandler(TargetPawnClass, ExtensionHandlerDelegate));
	}
}

void UGameFeatureAction_AddInputs::HandleActorExtension(AActor* Owner, const FName EventName)
{
	UE_LOG(LogGameplayFeaturesExtraActions, Display, TEXT("Event %s sent by Actor %s for inputs management."), *EventName.ToString(), *Owner->GetName());

	if (EventName == UGameFrameworkComponentManager::NAME_ExtensionRemoved || EventName == UGameFrameworkComponentManager::NAME_ReceiverRemoved)
	{
		RemoveActorInputs(Owner);
	}

	else if (EventName == UGameFrameworkComponentManager::NAME_ExtensionAdded || EventName == UGameFrameworkComponentManager::NAME_GameActorReady)
	{
		if (ActiveExtensions.Contains(Owner) || !ModularFeaturesHelper::ActorHasAllRequiredTags(Owner, RequireTags))
		{
			return;
		}

		if (InputMappingContext.IsNull())
		{
			UE_LOG(LogGameplayFeaturesExtraActions, Error, TEXT("%s: Input Mapping Context is null."), *FString(__func__));
		}
		else
		{
			AddActorInputs(Owner);
		}
	}
}

void UGameFeatureAction_AddInputs::AddActorInputs(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	APawn* const TargetPawn = Cast<APawn>(TargetActor);

	if (!IsValid(TargetPawn))
	{
		return;
	}

	if (const APlayerController* const PlayerController = TargetPawn->GetController<APlayerController>())
	{
		if (const ULocalPlayer* const LocalPlayer = PlayerController->GetLocalPlayer();
			PlayerController->IsLocalController() && IsValid(LocalPlayer))
		{
			UEnhancedInputLocalPlayerSubsystem* const Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

			if (!IsValid(Subsystem))
			{
				UE_LOG(LogGameplayFeaturesExtraActions, Error, TEXT("%s: LocalPlayer %s has no EnhancedInputLocalPlayerSubsystem."), *FString(__func__), *LocalPlayer->GetName());

				return;
			}

			FInputBindingData& NewInputData = ActiveExtensions.FindOrAdd(TargetActor);
			UInputMappingContext* const InputMapping = InputMappingContext.LoadSynchronous();

			UE_LOG(LogGameplayFeaturesExtraActions, Display, TEXT("%s: Adding Enhanced Input Mapping %s to Actor %s."), *FString(__func__), *InputMapping->GetName(), *TargetActor->GetName());

			Subsystem->AddMappingContext(InputMapping, MappingPriority);

			NewInputData.Mapping = InputMapping;

			const TWeakObjectPtr<UObject> FunctionOwner = ModularFeaturesHelper::GetPawnInputOwner(TargetPawn, PluginSettings->InputBindingOwner);
			const TWeakObjectPtr<UEnhancedInputComponent> InputComponent = ModularFeaturesHelper::GetEnhancedInputComponentInPawn(TargetPawn, PluginSettings->InputBindingOwner);

			if (!FunctionOwner.IsValid() || !InputComponent.IsValid())
			{
				UE_LOG(LogGameplayFeaturesExtraActions, Error, TEXT("%s: Failed to find InputComponent on Actor %s."), *FString(__func__), *TargetActor->GetName());
			}
			else
			{
				SetupActionBindings(TargetActor, FunctionOwner.Get(), InputComponent.Get());

				ActiveExtensions.Add(TargetActor, NewInputData);
			}
		}
	}
	else if (TargetPawn->IsPawnControlled())
	{
		UE_LOG(LogGameplayFeaturesExtraActions, Error, TEXT("%s: Failed to find PlayerController on Actor %s."), *FString(__func__), *TargetActor->GetName());
	}
}

void UGameFeatureAction_AddInputs::RemoveActorInputs(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		ActiveExtensions.Remove(TargetActor);
		return;
	}

	APawn* const TargetPawn = Cast<APawn>(TargetActor);

	if (!IsValid(TargetPawn))
	{
		ActiveExtensions.Remove(TargetActor);
		return;
	}

	if (const APlayerController* const PlayerController = TargetPawn->GetController<APlayerController>())
	{
		if (const ULocalPlayer* const LocalPlayer = PlayerController->GetLocalPlayer();
			PlayerController->IsLocalController() && IsValid(LocalPlayer))
		{
			UEnhancedInputLocalPlayerSubsystem* const Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

			if (!IsValid(Subsystem))
			{
				UE_LOG(LogGameplayFeaturesExtraActions, Error, TEXT("%s: LocalPlayer %s has no EnhancedInputLocalPlayerSubsystem."), *FString(__func__), *LocalPlayer->GetName());

				return;
			}

			if (const FInputBindingData* const ActiveInputData = ActiveExtensions.Find(TargetActor);
				ActiveInputData != nullptr)
			{
				UE_LOG(LogGameplayFeaturesExtraActions, Display, TEXT("%s: Removing Enhanced Input Mapping %s from Actor %s."), *FString(__func__), *ActiveInputData->Mapping->GetName(), *TargetActor->GetName());

				if (const TWeakObjectPtr<UEnhancedInputComponent> InputComponent = ModularFeaturesHelper::GetEnhancedInputComponentInPawn(TargetPawn, PluginSettings->InputBindingOwner);
					!InputComponent.IsValid())
				{
					UE_LOG(LogGameplayFeaturesExtraActions, Error, TEXT("%s: Failed to find InputComponent on Actor %s."), *FString(__func__), *TargetActor->GetName());
				}
				else
				{
					for (const FInputBindingHandle& InputActionBinding : ActiveInputData->ActionBinding)
					{
						InputComponent->RemoveBinding(InputActionBinding);
					}

					if (const IAbilityInputBinding* const SetupInputInterface = ModularFeaturesHelper::GetAbilityInputBindingInterface(TargetActor, PluginSettings->InputBindingOwner))
					{
						ModularFeaturesHelper::RemoveAbilityInputInInterfaceOwner(SetupInputInterface->_getUObject(), AbilityActions);
					}
				}

				Subsystem->RemoveMappingContext(ActiveInputData->Mapping.Get());
			}
		}
	}
	else if (TargetPawn->IsPawnControlled())
	{
		UE_LOG(LogGameplayFeaturesExtraActions, Error, TEXT("%s: Failed to find PlayerController on Actor %s."), *FString(__func__), *TargetActor->GetName());
	}

	ActiveExtensions.Remove(TargetActor);
}

void UGameFeatureAction_AddInputs::SetupActionBindings(AActor* TargetActor, UObject* FunctionOwner, UEnhancedInputComponent* InputComponent)
{
	FInputBindingData& NewInputData = ActiveExtensions.FindOrAdd(TargetActor);
	const IAbilityInputBinding* const SetupInputInterface = ModularFeaturesHelper::GetAbilityInputBindingInterface(TargetActor, PluginSettings->InputBindingOwner);

	// Initializing the enum class here in case of the use of enumeration class is enabled to avoid disk consumption due to loads inside a for-loop
	TWeakObjectPtr<UEnum> InputIDEnumeration_Ptr = ModularFeaturesHelper::LoadInputEnum(PluginSettings);

	for (const auto& [ActionInput, AbilityBindingData, FunctionBindingData] : ActionsBindings)
	{
		if (ActionInput.IsNull())
		{
			UE_LOG(LogGameplayFeaturesExtraActions, Error, TEXT("%s: Action Input is null."), *FString(__func__));
			continue;
		}

		UInputAction* const InputAction = ActionInput.LoadSynchronous();

		UE_LOG(LogGameplayFeaturesExtraActions, Display, TEXT("%s: Binding Action Input %s to Actor %s."), *FString(__func__), *InputAction->GetName(), *TargetActor->GetName());

		for (const auto& [FunctionName, Triggers] : FunctionBindingData)
		{
			for (const ETriggerEvent& Trigger : Triggers)
			{
				NewInputData.ActionBinding.Add(InputComponent->BindAction(InputAction, Trigger, FunctionOwner, FunctionName));
			}
		}

		if (!AbilityBindingData.bSetupAbilityInput)
		{
			continue;
		}

		const int32 InputID = PluginSettings->bUseInputEnumeration ? InputIDEnumeration_Ptr->GetValueByName(AbilityBindingData.InputIDValueName, EGetByNameFlags::CheckAuthoredName) : -1;
		if (ModularFeaturesHelper::BindAbilityInputToInterfaceOwner(SetupInputInterface, InputAction, InputID))
		{
			AbilityActions.Add(InputAction);
		}
	}
}
