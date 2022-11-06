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

UENUM(BlueprintType, Category = "MF Extra Actions | Enums")
enum class EAbilityBindingMode :uint8
{
	InputID,
	AbilitySpec,
	AbilityTags,
	AbilityClass
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Modular Features - Extra Actions"))
class MODULARFEATURES_EXTRAACTIONS_API UMFEA_Settings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	explicit UMFEA_Settings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/* Work in Progress: If true, will auto bind the ability input directly using the given Input Action */
	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Settings", Meta = (DisplayName = "Work in Progress: Enable Ability Auto Binding"))
	bool bEnableAbilityAutoBinding;

	/* Determine the binding mode that will be used by this plugin - This choice affects which function from IAbilityInputBinding interface will be used to bind the abilities */
	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Settings", Meta = (EditCondition = "!bEnableAbilityAutoBinding"))
	EAbilityBindingMode AbilityBindingMode;

	/* Enumeration class that will be used by the Ability System Component to manage abilities inputs if AbilityBindingMode is set to InputID */
	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Settings", Meta = (DisplayName = "InputID Enumeration", EditCondition = "!bEnableAbilityAutoBinding && AbilityBindingMode == EAbilityBindingMode::InputID"))
	TSoftObjectPtr<UEnum> InputIDEnumeration;

	/* Determine if the ability binding will be performed to the pawn or its controller - Must be the owner of the binding interface */
	UPROPERTY(GlobalConfig, EditAnywhere, Category = "Settings")
	EControllerOwner InputBindingOwner;
};
