// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#include "Actions/GameFeatureAction_SpawnActors.h"
#include "Components/GameFrameworkComponentManager.h"

void UGameFeatureAction_SpawnActors::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	if (!ensureAlways(SpawnedActors.IsEmpty()))
	{
		ResetExtension();
	}

	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (Context.ShouldApplyToWorldContext(WorldContext))
		{
			AddToWorld(WorldContext.World());
		}
	}

	WorldInitializedHandle = FWorldDelegates::OnPostWorldInitialization.AddUObject(this, &UGameFeatureAction_SpawnActors::OnWorldInitialized);
}

void UGameFeatureAction_SpawnActors::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	FWorldDelegates::OnPostWorldInitialization.Remove(WorldInitializedHandle);
	ResetExtension();
}

void UGameFeatureAction_SpawnActors::ResetExtension()
{
	DestroyActors();
}

void UGameFeatureAction_SpawnActors::OnWorldInitialized(UWorld* World, [[maybe_unused]] const UWorld::InitializationValues)
{
	AddToWorld(World);
}

void UGameFeatureAction_SpawnActors::AddToWorld(UWorld* World)
{
	if (TargetLevel.IsNull())
	{
		return;
	}

	if (World->IsGameWorld() && World->GetNetMode() != NM_Client && World->GetName() == TargetLevel.LoadSynchronous()->GetName())
	{
		SpawnActors(World);
	}
}

void UGameFeatureAction_SpawnActors::SpawnActors(UWorld* WorldReference)
{
	// Only proceed if the world is valid
	if (!IsValid(WorldReference))
	{
		return;
	}

	// Iterate through all spawn settings and spawn the actors with the given data
	for (const auto& [ActorClass, SpawnTransform] : SpawnSettings)
	{
		// Check if the soft reference is null
		if (ActorClass.IsNull())
		{
			UE_LOG(LogGameplayFeaturesExtraActions, Error, TEXT("%s: Actor class is null."), *FString(__func__));
			continue;
		}

		// Load the actor class and store it into a variable
		TSubclassOf<AActor> ClassToSpawn = ActorClass.LoadSynchronous();

		UE_LOG(LogGameplayFeaturesExtraActions, Display, TEXT("%s: Spawning actor %s on world %s"), *FString(__func__), *ClassToSpawn->GetName(), *WorldReference->GetName());

		// Spawn the actor and add it to the spawned array
		AActor* const SpawnedActor = WorldReference->SpawnActor<AActor>(ClassToSpawn, SpawnTransform);
		SpawnedActors.Add(SpawnedActor);
	}
}

void UGameFeatureAction_SpawnActors::DestroyActors()
{
	// Iterate through all spawned actors and destroy all valid actors
	for (const TWeakObjectPtr<AActor>& ActorPtr : SpawnedActors)
	{
		if (ActorPtr.IsValid())
		{
			ActorPtr->Destroy();
		}
	}

	SpawnedActors.Empty();
}
