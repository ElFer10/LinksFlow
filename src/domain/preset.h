#pragma once

#include "resolutionsettings.h"

#include <QString>

struct Preset
{
    QString id;
    QString name = QStringLiteral("Predeterminado");

    bool builtIn = true;

    ResolutionSettings resolution;
};