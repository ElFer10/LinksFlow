#include "adobeindesignbridge.h"

#include "adobebridgetransport.h"
#include "indesignanalysisparser.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

AdobeInDesignBridge::AdobeInDesignBridge(AdobeBridgeTransport *transport, QObject *parent)
    : InDesignBridge(parent), m_transport(transport)
{
    m_analysisTimeout.setSingleShot(true);
    m_analysisTimeout.setInterval(15000);

    connect(&m_analysisTimeout, &QTimer::timeout, this, &AdobeInDesignBridge::handleAnalysisTimeout);

    connect(m_transport, &AdobeBridgeTransport::textMessageReceived, this,
            [this](AdobeHost host, const QString &message) {
                if (host != AdobeHost::InDesign)
                {
                    return;
                }

                handleMessage(message);
            });

    connect(m_transport, &AdobeBridgeTransport::clientDisconnected, this, [this](AdobeHost host) {
        if (host != AdobeHost::InDesign)
        {
            return;
        }

        if (m_pendingAnalysisId.isEmpty())
        {
            return;
        }

        m_analysisTimeout.stop();
        m_pendingAnalysisId.clear();

        emit analysisFailed(tr("Se perdió la conexión con Adobe InDesign."));
    });

    // connect(m_transport, &AdobeBridgeTransport::clientDisconnected, this, [this]() {
    //     if (m_pendingAnalysisId.isEmpty())
    //     {
    //         return;
    //     }
    //
    //     m_analysisTimeout.stop();
    //     m_pendingAnalysisId.clear();
    //
    //     emit analysisFailed(QStringLiteral("Se perdió la conexión con Adobe InDesign."));
    // });
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

    // Los eventos como bridgeReady no son respuestas a una petición.
    const QString responseId = object.value(QStringLiteral("id")).toString();

    if (responseId.isEmpty() || responseId != m_pendingAnalysisId)
    {
        return;
    }

    m_analysisTimeout.stop();
    m_pendingAnalysisId.clear();

    const InDesignAnalysisParser::Result result = InDesignAnalysisParser::parse(message.toUtf8());

    if (!result.success)
    {
        emit analysisFailed(result.errorMessage);

        return;
    }

    emit analysisCompleted(result.links);
}

void AdobeInDesignBridge::handleAnalysisTimeout()
{
    if (m_pendingAnalysisId.isEmpty())
    {
        return;
    }

    m_pendingAnalysisId.clear();

    emit analysisFailed(QStringLiteral("Adobe InDesign no respondió a la solicitud de análisis."));
}
