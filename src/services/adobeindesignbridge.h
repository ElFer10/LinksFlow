#pragma once

#include "indesignbridge.h"

#include <QString>

class AdobeInDesignBridge : public InDesignBridge {
  Q_OBJECT

public:
  explicit AdobeInDesignBridge(QObject *parent = nullptr);

  void analyzeActiveDocument() override;

  void setAnalysisFilePath(const QString &filePath);

private:
  QString m_analysisFilePath;
};
