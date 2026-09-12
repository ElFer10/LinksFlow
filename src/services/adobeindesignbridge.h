#pragma once
#include <QTimer>

#include "indesignbridge.h"

class AdobeBridgeTransport;

class AdobeInDesignBridge : public InDesignBridge
{
    Q_OBJECT

  public:
    explicit AdobeInDesignBridge(AdobeBridgeTransport *transport, QObject *parent = nullptr);

    void analyzeActiveDocument() override;
    void updateLinks(const QList<qint64> &linkIds) override;

  private:
    void handleMessage(const QString &message);
    void handleAnalysisTimeout();

    AdobeBridgeTransport *m_transport = nullptr;
    void handleLinksUpdateTimeout();

    QString m_pendingLinksUpdateId;
    QTimer m_linksUpdateTimeout;
    QString m_pendingAnalysisId;
    QTimer m_analysisTimeout;
};
