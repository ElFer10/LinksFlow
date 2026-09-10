#pragma once

#include "linkinfo.h"

#include <QList>
#include <QString>

struct InDesignDocumentInfo
{
    qint64 id = 0;

    QString name;
    QString path;

    QList<LinkInfo> links;
};
