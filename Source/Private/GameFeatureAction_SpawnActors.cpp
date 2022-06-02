// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#include "GameFeatureAction_SpawnActors.h"
#include "Components/GameFrameworkComponentManager.h"

void UGameFeatureAction_SpawnActors::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	if (!ensureAlways(SpawnedActors.IsEmpty()))
	{
		ResetExtension();
	}

	Super::OnGameFeatureActivating(Context);
}

void UGameFeatureAction_SpawnActors::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);

	ResetExtension();
}

void UGameFeatureAction_SpawnActors::ResetExtension()
{
	DestroyActors();
}

void UGameFeatureAction_SpawnActors::AddToWorld(const FWorldContext& WorldContext)
{
	if (!TargetLevel.IsNull())
	{
		if (UWorld* World = WorldContext.World(); World->IsGameWorld() && World->GetNetMode() != NM_Client
			&& World->GetName() == TargetLevel.LoadSynchronous()->GetName())
		{
			SpawnActors(World);
		}
	}
}

void UGameFeatureAction_SpawnActors::SpawnActors(UWorld* WorldReference)
{
	for (const auto& [ActorClass, SpawnTransform] : SpawnSettings)
	{
		if (!ActorClass.IsNull())
		{
			TSubclassOf<AActor> ClassToSpawn = ActorClass.LoadSynchronous();

			UE_LOG(LogGameplayFeaturesExtraActions, Display,
			       TEXT("%s: Spawning actor %s on world %s"), *FString(__func__), *ClassToSpawn->GetName(),
			       *TargetLevel.GetAssetName());

			AActor* SpawnedActor = WorldReference->SpawnActor<AActor>(ClassToSpawn, SpawnTransform);
			SpawnedActors.Add(SpawnedActor);
		}
		else
		{
			UE_LOG(LogGameplayFeaturesExtraActions, Error, TEXT("%s: Actor class is null."), *FString(__func__));
		}
	}
}

void UGameFeatureAction_SpawnActors::DestroyActors()
{
	for (const TWeakObjectPtr<AActor>& ActorPtr : SpawnedActors)
	{
		if (ActorPtr.IsValid())
		{
			ActorPtr->Destroy();
		}
	}

	SpawnedActors.Empty();
}
