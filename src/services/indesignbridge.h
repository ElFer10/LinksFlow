#pragma once

#include <QList>
#include <QObject>
#include <QString>

#include "../domain/indesigndocumentinfo.h"
#include "../domain/linkinfo.h"
#include "../domain/linkupdateresult.h"

class InDesignBridge : public QObject
{
    Q_OBJECT

  public:
    explicit InDesignBridge(QObject *parent = nullptr) : QObject(parent)
    {
    }

    ~InDesignBridge() override = default;

    virtual void analyzeActiveDocument() = 0;
    virtual void updateLinks(const QList<qint64> &linkIds) = 0;

  signals:
    void analysisStarted();

    void analysisCompleted(const InDesignDocumentInfo &document);

    void analysisFailed(const QString &message);
    void linksUpdated(const LinksUpdateResult &result);
    void linksUpdateFailed(const QString &message);
};
