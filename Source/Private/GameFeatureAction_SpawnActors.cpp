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
		if (UWorld* World = WorldContext.World(); IsValid(World) && World->IsGameWorld() && World == TargetLevel.Get())
		{
			SpawnActors(World);
		}
	}
}

void UGameFeatureAction_SpawnActors::SpawnActors(UWorld* WorldReference)
{
	for (const auto& [ActorClass, SpawnTransform] : SpawnSettings)
	{
		UE_LOG(LogGameplayFeaturesExtraActions, Display,
		       TEXT("Spawning actor %s on world %s"), *ActorClass.GetAssetName(), *TargetLevel.GetAssetName());

		AActor* SpawnedActor = WorldReference->SpawnActor<AActor>(ActorClass.LoadSynchronous(),
		                                                          SpawnTransform);
		SpawnedActors.Add(SpawnedActor);
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
