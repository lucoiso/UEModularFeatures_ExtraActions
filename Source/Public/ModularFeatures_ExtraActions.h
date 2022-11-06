// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEModularFeatures_ExtraActions

#pragma once

#include "Modules/ModuleInterface.h"

/**
 *
 */

DECLARE_LOG_CATEGORY_EXTERN(LogGameplayFeaturesExtraActions, Display, All);

class FModularFeatures_ExtraActionsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};