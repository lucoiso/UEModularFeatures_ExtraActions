// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#include "GameFeatureAction_AddAttribute.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/GameFrameworkComponentManager.h"

void UGameFeatureAction_AddAttribute::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	if (!ensureAlways(ActiveExtensions.IsEmpty()) ||
		!ensureAlways(ActiveRequests.IsEmpty()))
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

	ActiveRequests.Empty();
}

void UGameFeatureAction_AddAttribute::AddToWorld(const FWorldContext& WorldContext)
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
					this, &UGameFeatureAction_AddAttribute::HandleActorExtension);

			const TSharedPtr<FComponentRequestHandle> RequestHandle =
				ComponentManager->AddExtensionHandler(TargetPawnClass, ExtensionHandlerDelegate);

			ActiveRequests.Add(RequestHandle);
		}
	}
}

void UGameFeatureAction_AddAttribute::HandleActorExtension(AActor* Owner, const FName EventName)
{
	/*UE_LOG(LogGameplayFeaturesExtraActions, Display,
		   TEXT("Event %s sended by Actor %s for attribute management."), *EventName.ToString(),
		   *Owner->GetName());*/

	if (EventName == UGameFrameworkComponentManager::NAME_ExtensionRemoved || EventName ==
		UGameFrameworkComponentManager::NAME_ReceiverRemoved)
	{
		RemoveAttribute(Owner);
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

		if (!Attribute.IsNull())
		{
			AddAttribute(Owner);
		}
		else
		{
			UE_LOG(LogGameplayFeaturesExtraActions, Error, TEXT("%s: Attribute is null."), *FString(__func__));
		}
	}
}

void UGameFeatureAction_AddAttribute::AddAttribute(AActor* TargetActor)
{
	if (IsValid(TargetActor) && TargetActor->GetLocalRole() == ROLE_Authority)
	{
		const IAbilitySystemInterface* InterfaceOwner = Cast<IAbilitySystemInterface>(TargetActor);

		if (UAbilitySystemComponent* AbilitySystemComponent = InterfaceOwner != nullptr
			                                                      ? InterfaceOwner->GetAbilitySystemComponent()
			                                                      : TargetActor->FindComponentByClass<
				                                                      UAbilitySystemComponent>(); IsValid(
			AbilitySystemComponent))
		{
			if (const TSubclassOf<UAttributeSet> SetType = Attribute.LoadSynchronous(); IsValid(SetType))
			{
				UAttributeSet* NewSet = NewObject<UAttributeSet>(AbilitySystemComponent->GetOwnerActor(), SetType);

				if (!InitializationData.IsNull())
				{
					NewSet->InitFromMetaDataTable(InitializationData.LoadSynchronous());
				}

				AbilitySystemComponent->AddAttributeSetSubobject(NewSet);
				AbilitySystemComponent->ForceReplication();

				UE_LOG(LogGameplayFeaturesExtraActions, Display,
				       TEXT("%s: Attribute %s added to Actor %s."), *FString(__func__), *SetType->GetName(),
				       *TargetActor->GetName());

				ActiveExtensions.Add(TargetActor, NewSet);
			}
			else
			{
				UE_LOG(LogGameplayFeaturesExtraActions, Error, TEXT("%s: Attribute is invalid."), *FString(__func__));
			}
		}
		else
		{
			UE_LOG(LogGameplayFeaturesExtraActions, Error,
			       TEXT("%s: Failed to find AbilitySystemComponent on Actor %s."), *FString(__func__),
			       *TargetActor->GetName());
		}
	}
}

void UGameFeatureAction_AddAttribute::RemoveAttribute(AActor* TargetActor)
{
	if (IsValid(TargetActor) && TargetActor->GetLocalRole() == ROLE_Authority)
	{
		const IAbilitySystemInterface* InterfaceOwner = Cast<IAbilitySystemInterface>(TargetActor);

		if (UAbilitySystemComponent* AbilitySystemComponent = InterfaceOwner != nullptr
			                                                      ? InterfaceOwner->GetAbilitySystemComponent()
			                                                      : TargetActor->FindComponentByClass<
				                                                      UAbilitySystemComponent>(); IsValid(
			AbilitySystemComponent))
		{
			if (UAttributeSet* AttributeToRemove = ActiveExtensions.FindRef(TargetActor).Get();
				IsValid(AttributeToRemove)
				&& AbilitySystemComponent->GetSpawnedAttributes_Mutable().Remove(AttributeToRemove) != 0)
			{
				UE_LOG(LogGameplayFeaturesExtraActions, Display, TEXT("%s: Attribute %s removed from Actor %s."),
				       *FString(__func__),
				       *AttributeToRemove->GetName(), *TargetActor->GetName());

				AbilitySystemComponent->ForceReplication();
			}
		}
		else if (TargetActor->GetNetOwningPlayer())
		{
			UE_LOG(LogGameplayFeaturesExtraActions, Error,
			       TEXT("%s: Failed to find AbilitySystemComponent on Actor %s."), *FString(__func__),
			       *TargetActor->GetName());
		}
	}

	ActiveExtensions.Remove(TargetActor);
}
