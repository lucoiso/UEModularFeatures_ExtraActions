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

void UGameFeatureAction_AddInputs::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	if (!ensureAlways(ActiveExtensions.IsEmpty()) ||
		!ensureAlways(ActiveRequests.IsEmpty()))
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
		const auto ExtensionIterator = ActiveExtensions.CreateIterator();
		RemoveActorInputs(ExtensionIterator->Key.Get());
	}

	ActiveRequests.Empty();
}

void UGameFeatureAction_AddInputs::AddToWorld(const FWorldContext& WorldContext)
{
	const UWorld* World = WorldContext.World();

	if (const UGameInstance* GameInstance = WorldContext.OwningGameInstance;
		IsValid(GameInstance) && IsValid(World) && World->IsGameWorld())
	{
		if (UGameFrameworkComponentManager* ComponentManager = UGameInstance::GetSubsystem<
			UGameFrameworkComponentManager>(GameInstance); IsValid(ComponentManager) && !TargetPawnClass.IsNull())
		{
			const UGameFrameworkComponentManager::FExtensionHandlerDelegate ExtensionHandlerDelegate =
				UGameFrameworkComponentManager::FExtensionHandlerDelegate::CreateUObject(
					this, &UGameFeatureAction_AddInputs::HandleActorExtension);

			const TSharedPtr<FComponentRequestHandle> RequestHandle =
				ComponentManager->AddExtensionHandler(TargetPawnClass, ExtensionHandlerDelegate);

			ActiveRequests.Add(RequestHandle);
		}
	}
}

void UGameFeatureAction_AddInputs::HandleActorExtension(AActor* Owner, const FName EventName)
{
	/*UE_LOG(LogGameplayFeaturesExtraActions, Warning,
		   TEXT("Event %s sended by Actor %s for inputs management."), *EventName.ToString(),
		   *Owner->GetName());*/

	if (EventName == UGameFrameworkComponentManager::NAME_ExtensionRemoved || EventName ==
		UGameFrameworkComponentManager::NAME_ReceiverRemoved)
	{
		RemoveActorInputs(Owner);
	}

	else if (EventName == UGameFrameworkComponentManager::NAME_ExtensionAdded || EventName ==
		UGameFrameworkComponentManager::NAME_GameActorReady)
	{
		if (ActiveExtensions.Contains(Owner))
		{
			return;
		}

		if (RequireTags.Num() != 0)
		{
			for (const FName Tag : RequireTags)
			{
				if (Owner->ActorHasTag(Tag))
				{
					return;
				}
			}
		}

		if (!InputMappingContext.IsNull())
		{
			AddActorInputs(Owner);
		}
	}
}

void UGameFeatureAction_AddInputs::AddActorInputs(AActor* TargetActor)
{
	if (IsValid(TargetActor))
	{
		UE_LOG(LogGameplayFeaturesExtraActions, Display,
		       TEXT("Adding Enhanced Input Mapping %s to Actor %s."), *InputMappingContext.GetAssetName(),
		       *TargetActor->GetName());

		APawn* TargetPawn = Cast<APawn>(TargetActor);

		if (const APlayerController* PlayerController = TargetPawn->GetController<APlayerController>(); IsValid(
			PlayerController))
		{
			if (const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer(); IsValid(LocalPlayer))
			{
				UEnhancedInputLocalPlayerSubsystem*
					Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

				if (!IsValid(Subsystem))
				{
					return;
				}

				FInputBindingData& NewInputData = ActiveExtensions.FindOrAdd(TargetActor);
				UInputMappingContext* InputMapping = InputMappingContext.LoadSynchronous();

				Subsystem->AddMappingContext(InputMapping, MappingPriority);

				NewInputData.Mapping = InputMapping;

				TWeakObjectPtr<UObject> FunctionOwner;
				TWeakObjectPtr<UEnhancedInputComponent> InputComponent;

				switch (InputBindingOwner)
				{
				case EControllerOwner::Pawn:
					FunctionOwner = TargetPawn;
					InputComponent = Cast<UEnhancedInputComponent>(TargetPawn->InputComponent.Get());
					break;

				case EControllerOwner::Controller:
					FunctionOwner = TargetPawn->GetController();
					InputComponent = Cast<UEnhancedInputComponent>(TargetPawn->GetController()->InputComponent.Get());
					break;

				default:
					FunctionOwner.Reset();
					InputComponent.Reset();
					break;
				}

				const IAbilityInputBinding* AbilityInterface = Cast<IAbilityInputBinding>(FunctionOwner);

				if (FunctionOwner.IsValid() && InputComponent.IsValid())
				{
					for (const auto [ActionInput, AbilityBindingData, FunctionBindingData] : ActionsBindings)
					{
						UE_LOG(LogGameplayFeaturesExtraActions, Display,
						       TEXT("Binding Action Input %s to Actor %s."), *ActionInput.GetAssetName(),
						       *TargetActor->GetName());

						if (!ActionInput.IsNull())
						{
							UInputAction* InputAction = ActionInput.LoadSynchronous();

							for (const auto& [FunctionName, Triggers] : FunctionBindingData)
							{
								for (const ETriggerEvent& Trigger : Triggers)
								{
									const FInputBindingHandle& InputBindingHandle = InputComponent->BindAction(
										InputAction, Trigger, FunctionOwner.Get(), FunctionName);

									NewInputData.ActionBinding.Add(InputBindingHandle);
								}
							}

							if (AbilityInterface != nullptr)
							{
								if (AbilityBindingData.bSetupAbilityInput)
								{
									const uint32 InputID =
										AbilityBindingData.InputIDEnumerationClass.LoadSynchronous()->
										                   GetValueByName(
											                   AbilityBindingData.InputIDValueName,
											                   EGetByNameFlags::CheckAuthoredName);

									IAbilityInputBinding::Execute_SetupAbilityInputBinding(
										AbilityInterface->_getUObject(), InputAction, InputID);

									AbilityActions.Add(InputAction);
								}
							}
						}
					}

					ActiveExtensions.Add(TargetActor, NewInputData);
				}
			}
		}
	}
}

void UGameFeatureAction_AddInputs::RemoveActorInputs(AActor* TargetActor)
{
	if (IsValid(TargetActor))
	{
		UE_LOG(LogGameplayFeaturesExtraActions, Display,
		       TEXT("Removing Enhanced Input Mapping %s from Actor %s."), *InputMappingContext.GetAssetName(),
		       *TargetActor->GetName());

		APawn* TargetPawn = Cast<APawn>(TargetActor);

		if (const APlayerController* PlayerController = TargetPawn->GetController<APlayerController>(); IsValid(
			PlayerController))
		{
			if (const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer(); IsValid(LocalPlayer))
			{
				UEnhancedInputLocalPlayerSubsystem*
					Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();

				if (!IsValid(Subsystem))
				{
					return;
				}

				const FInputBindingData ActiveInputData = ActiveExtensions.FindRef(TargetActor);

				if constexpr (&ActiveInputData != nullptr)
				{
					Subsystem->RemoveMappingContext(ActiveInputData.Mapping.Get());

					UObject* FunctionOwner;

					switch (InputBindingOwner)
					{
					case EControllerOwner::Pawn:
						FunctionOwner = TargetPawn;
						break;

					case EControllerOwner::Controller:
						FunctionOwner = TargetPawn->GetController();
						break;

					default:
						FunctionOwner = static_cast<UObject*>(nullptr);
						break;
					}

					if (UEnhancedInputComponent* InputComponent =
						Cast<UEnhancedInputComponent>(TargetPawn->InputComponent); IsValid(FunctionOwner) && IsValid(
						InputComponent))
					{
						for (const FInputBindingHandle& BindHandle : ActiveInputData.ActionBinding)
						{
							InputComponent->RemoveBindingByHandle(BindHandle.GetHandle());
						}

						if (const IAbilityInputBinding* AbilityInterface = Cast<IAbilityInputBinding>(FunctionOwner);
							AbilityInterface != nullptr)
						{
							for (TWeakObjectPtr<UInputAction> ActiveAbilityAction : AbilityActions)
							{
								IAbilityInputBinding::Execute_RemoveAbilityInputBinding(
									AbilityInterface->_getUObject(), ActiveAbilityAction.Get());
								ActiveAbilityAction.Reset();
							}
						}
					}
				}
			}
		}
	}

	ActiveExtensions.Remove(TargetActor);
}
