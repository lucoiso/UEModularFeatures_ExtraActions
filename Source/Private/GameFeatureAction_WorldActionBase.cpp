// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#include "GameFeatureAction_WorldActionBase.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "ModularFeatures_InternalFuncs.h"

DEFINE_LOG_CATEGORY(LogGameplayFeaturesExtraActions);

void UGameFeatureAction_WorldActionBase::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	Super::OnGameFeatureActivating(Context);

	PluginSettings = ModularFeaturesHelper::GetPluginSettings();

	if (!IsValid(PluginSettings))
	{
		UE_LOG(LogGameplayFeaturesExtraActions, Error, TEXT("%s: Failed to get plugin settings."), *FString(__func__));
	}

	GameInstanceStartHandle = FWorldDelegates::OnStartGameInstance.AddUObject(this, &UGameFeatureAction_WorldActionBase::HandleGameInstanceStart, FGameFeatureStateChangeContext(Context));

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
