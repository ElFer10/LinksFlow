#include "adobeindesignbridge.h"

#include "../domain/indesigndocumentinfo.h"
#include "../domain/linkupdateresult.h"
#include "adobebridgetransport.h"
#include "indesignanalysisparser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

AdobeInDesignBridge::AdobeInDesignBridge(AdobeBridgeTransport *transport, QObject *parent)
    : InDesignBridge(parent), m_transport(transport)
{
    m_analysisTimeout.setSingleShot(true);
    m_analysisTimeout.setInterval(15000);
    m_linksUpdateTimeout.setSingleShot(true);
    m_linksUpdateTimeout.setInterval(10000);

    connect(&m_linksUpdateTimeout, &QTimer::timeout, this, &AdobeInDesignBridge::handleLinksUpdateTimeout);

    connect(&m_analysisTimeout, &QTimer::timeout, this, &AdobeInDesignBridge::handleAnalysisTimeout);

    connect(m_transport, &AdobeBridgeTransport::textMessageReceived, this,
            [this](AdobeHost host, const QString &message) {
                if (host != AdobeHost::InDesign)
                    return;

                handleMessage(message);
            });
    connect(m_transport, &AdobeBridgeTransport::clientDisconnected, this, [this](AdobeHost host) {
        if (host != AdobeHost::InDesign)
            return;

        if (!m_pendingLinksUpdateId.isEmpty())
        {
            m_linksUpdateTimeout.stop();
            m_pendingLinksUpdateId.clear();

            emit linksUpdateFailed(tr("Se perdió la conexión con Adobe InDesign."));
        }

        if (!m_pendingAnalysisId.isEmpty())
        {
            m_analysisTimeout.stop();
            m_pendingAnalysisId.clear();

            emit analysisFailed(tr("Se perdió la conexión con Adobe InDesign."));
        }
    });
}

void AdobeInDesignBridge::updateLinks(const QList<qint64> &linkIds)
{
    if (!m_transport)
    {
        emit linksUpdateFailed(tr("El transporte de Adobe no está disponible."));

        return;
    }

    if (!m_transport->hasClient(AdobeHost::InDesign))
    {
        emit linksUpdateFailed(tr("Adobe InDesign no está conectado."));

        return;
    }

    if (!m_pendingLinksUpdateId.isEmpty())
    {
        emit linksUpdateFailed(tr("Ya hay una actualización de vínculos en curso."));

        return;
    }

    if (linkIds.isEmpty())
    {
        LinksUpdateResult result;

        result.requested = 0;
        result.updated = 0;

        emit linksUpdated(result);

        return;
    }

    m_pendingLinksUpdateId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    QJsonArray ids;

    for (qint64 id : linkIds)
    {
        ids.append(static_cast<double>(id));
    }

    QJsonObject request;

    request["version"] = 1;
    request["id"] = m_pendingLinksUpdateId;
    request["command"] = QStringLiteral("updateLinks");
    request["linkIds"] = ids;

    const QByteArray json = QJsonDocument(request).toJson(QJsonDocument::Compact);

    m_linksUpdateTimeout.start();

    m_transport->sendTextMessage(AdobeHost::InDesign, QString::fromUtf8(json));
}
void AdobeInDesignBridge::analyzeActiveDocument()
{
    if (!m_transport)
    {
        emit analysisFailed(QStringLiteral("El transporte de Adobe no está disponible."));
        return;
    }

    if (!m_transport->hasClient(AdobeHost::InDesign))
    {
        emit analysisFailed(QStringLiteral("LinksFlow no está conectado con Adobe InDesign."));
        return;
    }

    if (!m_pendingAnalysisId.isEmpty())
    {
        emit analysisFailed(QStringLiteral("Ya hay un análisis de InDesign en curso."));
        return;
    }

    m_pendingAnalysisId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    QJsonObject request;

    request.insert("version", 1);
    request.insert("id", m_pendingAnalysisId);
    request.insert("command", "analyzeDocument");

    const QByteArray json = QJsonDocument(request).toJson(QJsonDocument::Compact);

    emit analysisStarted();

    m_analysisTimeout.start();

    m_transport->sendTextMessage(AdobeHost::InDesign, QString::fromUtf8(json));
}

void AdobeInDesignBridge::handleMessage(const QString &message)
{
    QJsonParseError parseError;

    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        return;
    }

    const QJsonObject object = document.object();

    //
    // Los eventos como bridgeReady
    // no contienen id de petición.
    //

    const QString responseId = object.value(QStringLiteral("id")).toString();

    if (responseId.isEmpty())
    {
        return;
    }

    //
    // ========================================================
    // RESPUESTA DE updateLinks
    // ========================================================
    //
    // IMPORTANTE:
    // Este bloque debe evaluarse ANTES
    // de comprobar m_pendingAnalysisId.
    //

    if (!m_pendingLinksUpdateId.isEmpty() && responseId == m_pendingLinksUpdateId)
    {
        m_linksUpdateTimeout.stop();

        m_pendingLinksUpdateId.clear();

        const bool success = object.value(QStringLiteral("success")).toBool(false);

        if (!success)
        {
            QString error = object.value(QStringLiteral("error")).toString();

            if (error.isEmpty())
            {
                error = tr("Adobe InDesign no pudo "
                           "actualizar los vínculos.");
            }

            emit linksUpdateFailed(error);

            return;
        }

        const QJsonObject result = object.value(QStringLiteral("result")).toObject();

        LinksUpdateResult updateResult;

        updateResult.requested = result.value(QStringLiteral("requested")).toInt();
        updateResult.updated = result.value(QStringLiteral("updated")).toInt();

        const QJsonArray details = result.value(QStringLiteral("details")).toArray();

        for (const QJsonValue &value : details)
        {
            if (!value.isObject())
                continue;

            const QJsonObject detail = value.toObject();

            UpdatedLinkInfo info;

            info.linkId = detail.value(QStringLiteral("linkId")).toInteger();
            info.success = detail.value(QStringLiteral("success")).toBool(false);
            info.statusBefore = detail.value(QStringLiteral("statusBefore")).toString();
            info.statusAfter = detail.value(QStringLiteral("statusAfter")).toString();
            info.message = detail.value(QStringLiteral("error")).toString();
            const QJsonObject resolution = detail.value(QStringLiteral("effectiveResolution")).toObject();
            info.effectiveResolutionX = resolution.value(QStringLiteral("x")).toDouble();
            info.effectiveResolutionY = resolution.value(QStringLiteral("y")).toDouble();
            updateResult.links.append(info);
        }

        emit linksUpdated(updateResult);

        return;
    }

    // ========================================================
    // RESPUESTA DEL ANÁLISIS
    // ========================================================

    if (m_pendingAnalysisId.isEmpty() || responseId != m_pendingAnalysisId)
        return;

    m_analysisTimeout.stop();

    m_pendingAnalysisId.clear();

    const InDesignAnalysisParser::Result result = InDesignAnalysisParser::parse(message.toUtf8());

    if (!result.success)
    {
        emit analysisFailed(result.errorMessage);

        return;
    }

    InDesignDocumentInfo documentInfo;

    documentInfo.id = result.documentId;

    documentInfo.name = result.documentName;

    documentInfo.path = result.documentPath;

    documentInfo.links = result.links;

    emit analysisCompleted(documentInfo);
}

void AdobeInDesignBridge::handleLinksUpdateTimeout()
{
    if (m_pendingLinksUpdateId.isEmpty())
        return;

    m_pendingLinksUpdateId.clear();

    emit linksUpdateFailed(tr("Adobe InDesign no respondió a la actualización de vínculos."));
}

void AdobeInDesignBridge::handleAnalysisTimeout()
{
    if (m_pendingAnalysisId.isEmpty())
        return;

    m_pendingAnalysisId.clear();

    emit analysisFailed(QStringLiteral("Adobe InDesign no respondió a la solicitud de análisis."));
}
