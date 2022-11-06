// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MFEA_Settings.generated.h"

/**
 *
 */
UENUM(BlueprintType, Category = "MF Extra Actions | Enums")
enum class EControllerOwner :uint8
{
	Pawn,
	Controller
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Modular Features - Extra Actions: Plugin Settings"))
class MODULARFEATURES_EXTRAACTIONS_API UMFEA_Settings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	explicit UMFEA_Settings(const FObjectInitializer& ObjectInitializer);

	/* Work in Progress: If true, will auto bind the ability input directly using the given Input Action */
	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Settings")
	bool bEnableAbilityAutoBinding;

	/* Determine if we'll be using the InputID Enumeration Class or not to setup abilities. If false, SetupAbilityInputBinding will receive -1 as InputID value and you will need to bind the ability activation directly to the input action or activate the bEnableAutoBinding to let the plugin manage the bindings */
	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Settings", Meta = (DisplayName = "Use InputID Enumeration Class", EditCondition = "bEnableAbilityAutoBinding == false"))
	bool bUseInputEnumeration;

	/* Enumeration class that will be used by the Ability System Component to manage abilities inputs */
	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Settings", Meta = (DisplayName = "InputID Enumeration", EditCondition = "bEnableAbilityAutoBinding == false && bUseInputEnumeration == true"))
	TSoftObjectPtr<UEnum> InputIDEnumeration;

	/* Determine if the ability binding will be performed to the pawn or its controller */
	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Settings")
	EControllerOwner InputBindingOwner;
};
