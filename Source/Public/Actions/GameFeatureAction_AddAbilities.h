// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#pragma once

#include <CoreMinimal.h>
#include <GameplayAbilitySpec.h>
#include "Actions/GameFeatureAction_WorldActionBase.h"
#include "GameFeatureAction_AddAbilities.generated.h"

class UGameplayAbility;
class UInputAction;
struct FComponentRequestHandle;

/**
 *
 */
USTRUCT(BlueprintType, Category = "MF Extra Actions | Modular Structs")
struct FAbilityMapping
{
    GENERATED_BODY()

    /* Ability class to be added */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
    TSoftClassPtr<UGameplayAbility> AbilityClass;

    /* Enhanced Input Action to bind ability activation */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
    TSoftObjectPtr<UInputAction> InputAction;

    /* Ability Level */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
    int32 AbilityLevel = 1;

    /* If the plugin is using an enumeration class to setup abilities, we need to specify wich enum value this input binding will be associated by its display name - Can ignore if not using enums to manage ability inputs */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (DisplayName = "InputID Value Name"))
    FName InputIDValueName = NAME_None;
};

/**
 *
 */
UCLASS(MinimalAPI, meta = (DisplayName = "MF Extra Actions: Add Abilities"))
class UGameFeatureAction_AddAbilities final : public UGameFeatureAction_WorldActionBase
{
    GENERATED_BODY()

public:
    /* Target pawn to which abilities will be given */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (AllowedClasses = "/Script/Engine.Pawn", OnlyPlaceable = "true"))
    TSoftClassPtr<APawn> TargetPawnClass;

    /* Determines whether the binding will be performed within the controller class or within the pawn */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
    EInputBindingOwnerOverride InputBindingOwnerOverride = EInputBindingOwnerOverride::Default;

    /* Tags required on the target to apply this action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
    TArray<FName> RequireTags;

    /* Abilities to be added */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings", meta = (DisplayName = "Ability Mapping", ShowOnlyInnerProperties))
    TArray<FAbilityMapping> Abilities;

protected:
    virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
    virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
    virtual void AddToWorld(const FWorldContext& WorldContext) override;

private:
    void HandleActorExtension(AActor* Owner, FName EventName);
    void ResetExtension();

    void AddActorAbilities(AActor* TargetActor, const FAbilityMapping& Ability);
    void RemoveActorAbilities(AActor* TargetActor);

    struct FActiveAbilityData
    {
        TArray<FGameplayAbilitySpecHandle> SpecHandle;
        TArray<TWeakObjectPtr<UInputAction>> InputReference;
    };

    TMap<TWeakObjectPtr<AActor>, FActiveAbilityData> ActiveExtensions;
    TWeakObjectPtr<UEnum> InputIDEnumeration_Ptr;
};
