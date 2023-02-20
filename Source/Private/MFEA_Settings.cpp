// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#pragma once

#include "MFEA_Settings.h"
#include "LogModularFeatures_ExtraActions.h"

#ifdef UE_INLINE_GENERATED_CPP_BY_NAME
#include UE_INLINE_GENERATED_CPP_BY_NAME(MFEA_Settings)
#endif

UMFEA_Settings::UMFEA_Settings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer), bEnableAbilityAutoBinding(false), bEnableInternalLogs(false), AbilityBindingMode(EAbilityBindingMode::InputID), InputBindingOwner(EInputBindingOwner::Controller)
{
	CategoryName = TEXT("Plugins");
}

const UMFEA_Settings* UMFEA_Settings::Get()
{
	static const UMFEA_Settings* const Instance = GetDefault<UMFEA_Settings>();
	return Instance;
}

#if WITH_EDITOR
void UMFEA_Settings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UMFEA_Settings, bEnableInternalLogs))
	{
		ToggleInternalLogs();
	}
}
#endif

void UMFEA_Settings::PostInitProperties()
{
	Super::PostInitProperties();
	ToggleInternalLogs();
}

void UMFEA_Settings::ToggleInternalLogs()
{
#if !UE_BUILD_SHIPPING
	LogGameplayFeaturesExtraActions_Internal.SetVerbosity(bEnableInternalLogs ? ELogVerbosity::Display : ELogVerbosity::NoLogging);
#endif
}
