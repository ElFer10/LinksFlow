#include "adobeindesignbridge.h"

#include "indesignanalysisparser.h"

#include <QFile>

AdobeInDesignBridge::AdobeInDesignBridge(QObject *parent)
    : InDesignBridge(parent) {}

void AdobeInDesignBridge::setAnalysisFilePath(const QString &filePath) {
  m_analysisFilePath = filePath;
}

void AdobeInDesignBridge::analyzeActiveDocument() {
  emit analysisStarted();

  if (m_analysisFilePath.isEmpty()) {
    emit analysisFailed(QStringLiteral("No se ha configurado el archivo "
                                       "de análisis de InDesign."));

    return;
  }

  QFile file(m_analysisFilePath);

  if (!file.open(QIODevice::ReadOnly)) {
    emit analysisFailed(QStringLiteral("No se pudo abrir el archivo "
                                       "de análisis de InDesign:\n%1")
                            .arg(m_analysisFilePath));

    return;
  }

  const QByteArray json = file.readAll();

  file.close();

  const InDesignAnalysisParser::Result result =
      InDesignAnalysisParser::parse(json);

  if (!result.success) {
    emit analysisFailed(result.errorMessage);

    return;
  }

  emit analysisCompleted(result.links);
}
