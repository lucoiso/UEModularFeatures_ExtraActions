// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#pragma once

#include "CoreMinimal.h"
#include "GameFeatureAction_WorldActionBase.h"
#include "GameFeatureAction_SpawnActors.generated.h"

struct FComponentRequestHandle;
/**
 *
 */
USTRUCT(BlueprintType, Category = "MF Extra Actions | Modular Structs")
struct FActorSpawnSettings
{
	GENERATED_BODY()

	/* Actor class to be spawned */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (OnlyPlaceable = "true"))
	TSoftClassPtr<AActor> ActorClass;

	/* Transform settings to be added to spawned actor */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	FTransform SpawnTransform;
};

/**
 *
 */
UCLASS(MinimalAPI, meta = (DisplayName = "MF Extra Actions: Spawn Actors"))
class UGameFeatureAction_SpawnActors final : public UGameFeatureAction
{
	GENERATED_BODY()

public:
	/* Target level to which actor will be spawned */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TSoftObjectPtr<UWorld> TargetLevel;

	/* Stacked spawn settings */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TArray<FActorSpawnSettings> SpawnSettings;

protected:
	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;

	void OnWorldInitialized(UWorld* World, const UWorld::InitializationValues InitializationValues);

private:
	void AddToWorld(UWorld* World);
	void SpawnActors(UWorld* WorldReference);
	void DestroyActors();

	void ResetExtension();

	TArray<TWeakObjectPtr<AActor>> SpawnedActors;
	FDelegateHandle WorldInitializedHandle;
};
