// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#include "Actions/GameFeatureAction_WorldActionBase.h"
#include <Engine/GameInstance.h>

#ifdef UE_INLINE_GENERATED_CPP_BY_NAME
#include UE_INLINE_GENERATED_CPP_BY_NAME(GameFeatureAction_WorldActionBase)
#endif

void UGameFeatureAction_WorldActionBase::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	Super::OnGameFeatureActivating(Context);

	if (!ensureAlways(ActiveRequests.IsEmpty()))
	{
		ResetExtension();
	}

	const FGameFeatureStateChangeContext StateChangeContext(Context);

	// When the game instance starts, will perform the modular feature activation behavior
	GameInstanceStartHandle = FWorldDelegates::OnStartGameInstance.AddUObject(this, &UGameFeatureAction_WorldActionBase::HandleGameInstanceStart, StateChangeContext);

	// Useful to activate the feature even if the game instance has already started
	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (!Context.ShouldApplyToWorldContext(WorldContext))
		{
			continue;
		}

		AddToWorld(WorldContext);
	}
}

void UGameFeatureAction_WorldActionBase::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);

	FWorldDelegates::OnStartGameInstance.Remove(GameInstanceStartHandle);
}

void UGameFeatureAction_WorldActionBase::ResetExtension()
{
	ActiveRequests.Empty();
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
	if (!ChangeContext.ShouldApplyToWorldContext(*GameInstance->GetWorldContext()))
	{
		return;
	}

	AddToWorld(*GameInstance->GetWorldContext());
}
