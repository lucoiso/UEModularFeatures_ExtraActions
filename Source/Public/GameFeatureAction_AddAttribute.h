// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#pragma once

#include "CoreMinimal.h"
#include "GameFeatureAction_WorldActionBase.h"
#include "GameFeatureAction_AddAttribute.generated.h"

class UAttributeSet;
class UDataTable;
struct FComponentRequestHandle;

/**
 *
 */
UCLASS(MinimalAPI, meta = (DisplayName = "MF Extra Actions: Add Attribute"))
class UGameFeatureAction_AddAttribute final : public UGameFeatureAction_WorldActionBase
{
	GENERATED_BODY()

public:
	/* Target pawn to which attribute will be given */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowedClasses = "/Script/Engine.Pawn", OnlyPlaceable = "true"))
	TSoftClassPtr<APawn> TargetPawnClass;

	/* Tags required on the target to apply this action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TArray<FName> RequireTags;

	/* AttributeSet class to be added */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TSoftClassPtr<UAttributeSet> Attribute;

	/* Data Table with Attribute Meta Data to be added (can be left unset) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TSoftObjectPtr<UDataTable> InitializationData;

protected:
	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
	virtual void AddToWorld(const FWorldContext& WorldContext) override;

private:
	void HandleActorExtension(AActor* Owner, FName EventName);
	void ResetExtension();

	void AddAttribute(AActor* TargetActor);
	void RemoveAttribute(AActor* TargetActor);

	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<UAttributeSet>> ActiveExtensions;
};
