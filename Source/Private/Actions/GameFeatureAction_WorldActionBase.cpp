// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#include "Actions/GameFeatureAction_WorldActionBase.h"

void UGameFeatureAction_WorldActionBase::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	Super::OnGameFeatureActivating(Context);

	// When the game instance starts, will perform the modular feature activation behavior
	GameInstanceStartHandle = FWorldDelegates::OnStartGameInstance.AddUObject(this, &UGameFeatureAction_WorldActionBase::HandleGameInstanceStart, FGameFeatureStateChangeContext(Context));

	// Useful to activate the feature even if the game instance has already started
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (Context.ShouldApplyToWorldContext(WorldContext))
		{
			AddToWorld(WorldContext);
		}
	}
}

void UGameFeatureAction_WorldActionBase::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);

	FWorldDelegates::OnStartGameInstance.Remove(GameInstanceStartHandle);
}

UGameFrameworkComponentManager* UGameFeatureAction_WorldActionBase::GetGameFrameworkComponentManager(const FWorldContext& WorldContext) const
{
	if (!IsValid(WorldContext.World()) || !WorldContext.World()->IsGameWorld())
	{
		return nullptr;
	}

	return UGameInstance::GetSubsystem<UGameFrameworkComponentManager>(WorldContext.OwningGameInstance);
}

void UGameFeatureAction_WorldActionBase::HandleGameInstanceStart(UGameInstance* GameInstance, const FGameFeatureStateChangeContext ChangeContext)
{
	if (ChangeContext.ShouldApplyToWorldContext(*GameInstance->GetWorldContext()))
	{
		AddToWorld(*GameInstance->GetWorldContext());
	}
}
