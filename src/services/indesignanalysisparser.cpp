#include "indesignanalysisparser.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

InDesignAnalysisParser::Result
InDesignAnalysisParser::parse(const QByteArray &json) {
  Result result;

  QJsonParseError parseError;

  const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);

  if (parseError.error != QJsonParseError::NoError) {
    result.errorMessage =
        QStringLiteral("JSON inválido: %1").arg(parseError.errorString());

    return result;
  }

  if (!document.isObject()) {
    result.errorMessage = QStringLiteral("La respuesta de InDesign "
                                         "no contiene un objeto JSON.");

    return result;
  }

  const QJsonObject root = document.object();

  const bool success = root.value(QStringLiteral("success")).toBool(false);

  if (!success) {
    result.errorMessage =
        root.value(QStringLiteral("error"))
            .toString(QStringLiteral("InDesign devolvió un error."));

    return result;
  }

  result.documentName = root.value(QStringLiteral("documentName")).toString();

  const QJsonArray linksArray = root.value(QStringLiteral("links")).toArray();

  result.links.reserve(linksArray.size());

  for (const QJsonValue &value : linksArray) {
    if (!value.isObject()) {
      continue;
    }

    result.links.append(parseLink(value.toObject()));
  }

  result.success = true;

  return result;
}

LinkInfo InDesignAnalysisParser::parseLink(const QJsonObject &object) {
  LinkInfo link;

  link.indesignLinkId = object.value("linkId").toInteger();

  link.indesignPageItemId = object.value("pageItemId").toInteger();

  link.page = object.value("page").toString();

  link.fileName = object.value("fileName").toString();

  link.filePath = object.value("filePath").toString();

  const QFileInfo fileInfo(link.filePath);

  if (fileInfo.exists() && fileInfo.isFile()) {
    link.fileSizeBytes = fileInfo.size();
  }

  if (!link.filePath.isEmpty() && (!fileInfo.exists() || !fileInfo.isFile())) {
    link.fileSizeBytes = 0;
  }

  link.fileType = object.value("fileType").toString();

  link.colorMode = object.value("colorMode").toString();

  link.iccProfile = object.value("iccProfile").toString();

  const QJsonObject actual = object.value("actualResolution").toObject();

  link.actualResolution = {actual.value("x").toDouble(),
                           actual.value("y").toDouble()};

  const QJsonObject effective = object.value("effectiveResolution").toObject();

  link.effectiveResolution = {effective.value("x").toDouble(),
                              effective.value("y").toDouble()};

  const QJsonObject scale = object.value("scale").toObject();

  link.scale = {scale.value("horizontal").toDouble(100.0),

                scale.value("vertical").toDouble(100.0)};

  link.rotation = object.value("rotation").toDouble();

  const QJsonObject flip = object.value("flip").toObject();

  link.flip = {flip.value("horizontal").toBool(false),

               flip.value("vertical").toBool(false)};

  link.state = parseState(object.value("state").toString());

  link.process = link.state == LinkProcessState::Ready;

  link.statusMessage = object.value("statusMessage").toString();

  return link;
}

LinkProcessState InDesignAnalysisParser::parseState(const QString &state) {
  if (state == QStringLiteral("missing")) {
    return LinkProcessState::Missing;
  }

  if (state == QStringLiteral("unsupported")) {
    return LinkProcessState::Unsupported;
  }

  if (state == QStringLiteral("error")) {
    return LinkProcessState::Error;
  }

  return LinkProcessState::Ready;
}
