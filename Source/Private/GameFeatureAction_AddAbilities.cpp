// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#include "GameFeatureAction_AddAbilities.h"
#include "AbilityInputBinding.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Engine/AssetManager.h"
#include "InputAction.h"

void UGameFeatureAction_AddAbilities::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	if (!ensureAlways(ActiveExtensions.IsEmpty()) ||
		!ensureAlways(ActiveRequests.IsEmpty()))
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
		const auto ExtensionIterator = ActiveExtensions.CreateIterator();
		RemoveActorAbilities(ExtensionIterator->Key.Get());
	}

	ActiveRequests.Empty();
}

void UGameFeatureAction_AddAbilities::AddToWorld(const FWorldContext& WorldContext)
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
					this, &UGameFeatureAction_AddAbilities::HandleActorExtension);

			const TSharedPtr<FComponentRequestHandle> RequestHandle =
				ComponentManager->AddExtensionHandler(TargetPawnClass, ExtensionHandlerDelegate);

			ActiveRequests.Add(RequestHandle);
		}
	}
}

void UGameFeatureAction_AddAbilities::HandleActorExtension(AActor* Owner, const FName EventName)
{
	/*UE_LOG(LogGameplayFeaturesExtraActions, Warning,
		   TEXT("Event %s sended by Actor %s for ability management."), *EventName.ToString(),
		   *Owner->GetName());*/

	if (EventName == UGameFrameworkComponentManager::NAME_ExtensionRemoved || EventName ==
		UGameFrameworkComponentManager::NAME_ReceiverRemoved)
	{
		RemoveActorAbilities(Owner);
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

		for (const FAbilityMapping& Entry : Abilities)
		{
			if (!Entry.AbilityClass.IsNull())
			{
				AddActorAbilities(Owner, Entry);
			}
		}
	}
}

void UGameFeatureAction_AddAbilities::AddActorAbilities(AActor* TargetActor,
                                                        const FAbilityMapping& Ability)
{
	if (IsValid(TargetActor) && TargetActor->GetLocalRole() == ROLE_Authority)
	{
		UE_LOG(LogGameplayFeaturesExtraActions, Display,
		       TEXT("Adding ability %s to Actor %s."), *Ability.AbilityClass.GetAssetName(),
		       *TargetActor->GetName());

		const IAbilitySystemInterface* InterfaceOwner = Cast<IAbilitySystemInterface>(TargetActor);

		if (UAbilitySystemComponent* AbilitySystemComponent = InterfaceOwner != nullptr
			                                                      ? InterfaceOwner->GetAbilitySystemComponent()
			                                                      : TargetActor->FindComponentByClass<
				                                                      UAbilitySystemComponent>(); IsValid(
			AbilitySystemComponent))
		{
			FActiveAbilityData NewAbilityData = ActiveExtensions.FindOrAdd(TargetActor);

			const uint32 InputID = InputIDEnumerationClass.LoadSynchronous()->GetValueByName(
				Ability.InputIDValueName, EGetByNameFlags::CheckAuthoredName);

			const FGameplayAbilitySpec NewAbilitySpec =
				FGameplayAbilitySpec(Ability.AbilityClass.LoadSynchronous(),
				                     Ability.AbilityLevel,
				                     InputID,
				                     TargetActor);

			if (const FGameplayAbilitySpecHandle NewSpecHandle = AbilitySystemComponent->GiveAbility(NewAbilitySpec);
				NewSpecHandle.IsValid())
			{
				NewAbilityData.SpecHandle.Add(NewSpecHandle);

				if (!Ability.InputAction.IsNull())
				{
					if (APawn* TargetPawn = Cast<APawn>(TargetActor); IsValid(TargetPawn))
					{
						IAbilityInputBinding* SetupInputInterface;
						switch (InputBindingOwner)
						{
						case EControllerOwner::Pawn:
							SetupInputInterface = Cast<IAbilityInputBinding>(TargetPawn);
							break;

						case EControllerOwner::Controller:
							SetupInputInterface = Cast<IAbilityInputBinding>(TargetPawn->GetController());
							break;

						default:
							SetupInputInterface = static_cast<IAbilityInputBinding*>(nullptr);
							break;
						}

						if (SetupInputInterface != nullptr)
						{
							UInputAction* AbilityInput = Ability.InputAction.LoadSynchronous();
							IAbilityInputBinding::Execute_SetupAbilityInputBinding(
								SetupInputInterface->_getUObject(), AbilityInput, InputID);

							NewAbilityData.InputReference.Add(AbilityInput);
						}
					}
				}

				ActiveExtensions.Add(TargetActor, NewAbilityData);
			}
		}
	}
}

void UGameFeatureAction_AddAbilities::RemoveActorAbilities(AActor* TargetActor)
{
	if (IsValid(TargetActor) && TargetActor->GetLocalRole() == ROLE_Authority)
	{
		UE_LOG(LogGameplayFeaturesExtraActions, Display,
		       TEXT("Removing associated abilities from Actor %s."),
		       *TargetActor->GetName());

		const FActiveAbilityData ActiveAbilities = ActiveExtensions.FindRef(TargetActor);

		if constexpr (&ActiveAbilities != nullptr)
		{
			const IAbilitySystemInterface* InterfaceOwner = Cast<IAbilitySystemInterface>(TargetActor);

			if (UAbilitySystemComponent* AbilitySystemComponent = InterfaceOwner != nullptr
				                                                      ? InterfaceOwner->GetAbilitySystemComponent()
				                                                      : TargetActor->FindComponentByClass<
					                                                      UAbilitySystemComponent>(); IsValid(
				AbilitySystemComponent))
			{
				for (const FGameplayAbilitySpecHandle& SpecHandle : ActiveAbilities.SpecHandle)
				{
					if (SpecHandle.IsValid())
					{
						AbilitySystemComponent->SetRemoveAbilityOnEnd(SpecHandle);
					}
				}

				if (APawn* TargetPawn = Cast<APawn>(TargetActor); IsValid(TargetPawn))
				{
					IAbilityInputBinding* SetupInputInterface;
					switch (InputBindingOwner)
					{
					case EControllerOwner::Pawn:
						SetupInputInterface = Cast<IAbilityInputBinding>(TargetPawn);
						break;

					case EControllerOwner::Controller:
						SetupInputInterface = Cast<IAbilityInputBinding>(TargetPawn->GetController());
						break;

					default:
						SetupInputInterface = static_cast<IAbilityInputBinding*>(nullptr);
						break;
					}

					if (SetupInputInterface != nullptr)
					{
						for (const UInputAction* InputRef : ActiveAbilities.InputReference)
						{
							if (IsValid(InputRef))
							{
								IAbilityInputBinding::Execute_RemoveAbilityInputBinding(
									SetupInputInterface->_getUObject(), InputRef);
							}
						}
					}
				}
			}
		}
	}

	ActiveExtensions.Remove(TargetActor);
}
