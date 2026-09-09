#pragma once

#include "linkinfo.h"

#include <QString>

struct ImageUsage
{
    qint64 indesignLinkId = 0;
    qint64 indesignPageItemId = 0;

    QString page;

    Resolution2D actualResolution;
    Resolution2D effectiveResolution;

    Scale2D scale;

    double rotation = 0.0;

    FlipState flip;
};
