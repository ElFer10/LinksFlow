#pragma once

#include "resolutionsettings.h"
#include "imageeditingsettings.h"

#include <QString>

struct Preset
{
    QString id;
    QString name = QStringLiteral("Predeterminado");

    bool builtIn = true;

    ResolutionSettings resolution;
    ImageEditingSettings imageEditing;
};
