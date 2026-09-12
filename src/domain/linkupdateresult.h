#pragma once

#include <QList>
#include <QString>

struct UpdatedLinkInfo
{
    qint64 linkId = 0;

    bool success = false;

    QString statusBefore;
    QString statusAfter;

    double effectiveResolutionX = 0.0;
    double effectiveResolutionY = 0.0;

    QString message;
};

struct LinksUpdateResult
{
    int requested = 0;
    int updated = 0;

    QList<UpdatedLinkInfo> links;
};
