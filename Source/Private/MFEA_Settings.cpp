// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#pragma once

#include "MFEA_Settings.h"

#ifdef UE_INLINE_GENERATED_CPP_BY_NAME
#include UE_INLINE_GENERATED_CPP_BY_NAME(MFEA_Settings)
#endif

UMFEA_Settings::UMFEA_Settings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer), bEnableAbilityAutoBinding(false), AbilityBindingMode(EAbilityBindingMode::InputID), InputBindingOwner(EInputBindingOwner::Controller)
{
	CategoryName = TEXT("Plugins");
}
