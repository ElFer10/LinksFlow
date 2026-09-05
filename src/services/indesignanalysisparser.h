#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QString>

#include "../domain/linkinfo.h"

class InDesignAnalysisParser {
public:
  struct Result {
    bool success = false;
    QString errorMessage;
    QString documentName;
    QList<LinkInfo> links;
  };

  static Result parse(const QByteArray &json);

private:
  static LinkInfo parseLink(const QJsonObject &object);

  static LinkProcessState parseState(const QString &state);
};
