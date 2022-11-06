// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#pragma once

#include "MFEA_Settings.h"

UMFEA_Settings::UMFEA_Settings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer), bEnableAbilityAutoBinding(false), AbilityBindingMode(EAbilityBindingMode::InputID), InputBindingOwner(EControllerOwner::Controller)
{
}
