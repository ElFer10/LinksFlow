#pragma once

#include "conversionsettings.h"
#include "imageeditingsettings.h"
#include "resolutionsettings.h"

struct OptimizationSettings
{
    ResolutionSettings resolution;
    ImageEditingSettings imageEditing;
    ConversionSettings conversion;
};
