#pragma once
#include <QTimer>

#include "indesignbridge.h"

class AdobeBridgeTransport;

class AdobeInDesignBridge : public InDesignBridge {
  Q_OBJECT

public:
  explicit AdobeInDesignBridge(AdobeBridgeTransport *transport,
                               QObject *parent = nullptr);

  void analyzeActiveDocument() override;

private:
  void handleMessage(const QString &message);
  void handleAnalysisTimeout();

  AdobeBridgeTransport *m_transport = nullptr;

  QString m_pendingAnalysisId;
  QTimer m_analysisTimeout;
};
