#pragma once

#include <QList>
#include <QObject>
#include <QString>

#include "../domain/linkinfo.h"

class InDesignBridge : public QObject {
  Q_OBJECT

public:
  explicit InDesignBridge(QObject *parent = nullptr) : QObject(parent) {}

  ~InDesignBridge() override = default;

  virtual void analyzeActiveDocument() = 0;

signals:
  void analysisStarted();

  void analysisCompleted(const QList<LinkInfo> &links);

  void analysisFailed(const QString &message);
};
