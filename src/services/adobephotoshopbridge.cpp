#include "adobephotoshopbridge.h"

#include "adobebridgetransport.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

AdobePhotoshopBridge::AdobePhotoshopBridge(AdobeBridgeTransport *transport, QObject *parent)
    : QObject(parent), m_transport(transport)
{
    m_pingTimeout.setSingleShot(true);

    m_pingTimeout.setInterval(5000);

    connect(&m_pingTimeout, &QTimer::timeout, this, &AdobePhotoshopBridge::handlePingTimeout);

    if (!m_transport)
    {
        return;
    }

    //
    // Solo escuchamos mensajes procedentes del cliente Photoshop.
    //

    connect(m_transport, &AdobeBridgeTransport::textMessageReceived, this,
            [this](AdobeHost host, const QString &message) {
                if (host != AdobeHost::Photoshop)
                {
                    return;
                }

                handleMessage(message);
            });

    //
    // Si Photoshop desaparece durante un ping pendiente, cancelamos inmediatamente la solicitud.
    //

    connect(m_transport, &AdobeBridgeTransport::clientDisconnected, this, [this](AdobeHost host) {
        if (host != AdobeHost::Photoshop)
        {
            return;
        }

        if (m_pendingPingId.isEmpty())
        {
            return;
        }

        m_pingTimeout.stop();

        m_pendingPingId.clear();

        emit pingFailed(tr("Se perdió la conexión con Adobe Photoshop."));
    });

    m_inspectionTimeout.setSingleShot(true);

    m_inspectionTimeout.setInterval(15000);

    connect(&m_inspectionTimeout, &QTimer::timeout, this, &AdobePhotoshopBridge::handleInspectionTimeout);
}

bool AdobePhotoshopBridge::isConnected() const
{
    return m_transport && m_transport->hasClient(AdobeHost::Photoshop);
}

void AdobePhotoshopBridge::ping()
{
    if (!m_transport)
    {
        emit pingFailed(tr("Adobe Bridge no está disponible."));

        return;
    }

    if (!m_transport->hasClient(AdobeHost::Photoshop))
    {
        emit pingFailed(tr("Adobe Photoshop no está conectado."));

        return;
    }

    if (!m_pendingPingId.isEmpty())
    {
        emit pingFailed(tr("Ya hay una solicitud pendiente para Adobe Photoshop."));

        return;
    }

    m_pendingPingId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    QJsonObject request;

    request["version"] = 1;
    request["id"] = m_pendingPingId;

    request["command"] = QStringLiteral("ping");

    const QString message = QString::fromUtf8(QJsonDocument(request).toJson(QJsonDocument::Compact));

    m_transport->sendTextMessage(AdobeHost::Photoshop, message);

    m_pingTimeout.start();
}

void AdobePhotoshopBridge::handleInspectionTimeout()
{
    if (m_pendingInspectionId.isEmpty())
    {
        return;
    }

    m_pendingInspectionId.clear();

    emit imageInspectionFailed(tr("Adobe Photoshop no respondió "
                                  "a la inspección de la imagen."));
}

void AdobePhotoshopBridge::handleMessage(const QString &message)
{
    QJsonParseError parseError;

    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        return;
    }

    const QJsonObject object = document.object();

    const QString responseId = object.value("id").toString();

    //
    // Respuesta a ping
    //

    if (!m_pendingPingId.isEmpty() && responseId == m_pendingPingId)
    {
        m_pingTimeout.stop();
        m_pendingPingId.clear();

        const bool success = object.value("success").toBool(false);

        if (!success)
        {

            QString error = object.value("error").toString();

            if (error.isEmpty())
            {
                error = tr("Adobe Photoshop respondió "
                           "con un error.");
            }

            emit pingFailed(error);

            return;
        }

        const QString result = object.value("result").toString();

        if (result != QStringLiteral("pong"))
        {
            emit pingFailed(tr("Adobe Photoshop devolvió "
                               "una respuesta inesperada."));

            return;
        }

        emit pingSucceeded();

        return;
    }

    //
    // Respuesta a inspectImage
    //

    if (!m_pendingInspectionId.isEmpty() && responseId == m_pendingInspectionId)
    {
        m_inspectionTimeout.stop();

        m_pendingInspectionId.clear();

        const bool success = object.value("success").toBool(false);

        if (!success)
        {

            QString error = object.value("error").toString();

            if (error.isEmpty())
            {
                error = tr("Adobe Photoshop no pudo inspeccionar la imagen.");
            }

            emit imageInspectionFailed(error);

            return;
        }

        const QJsonObject result = object.value("result").toObject();

        PhotoshopImageInfo info;

        info.name = result.value("name").toString();

        info.path = result.value("path").toString();

        info.width = result.value("width").toDouble();

        info.height = result.value("height").toDouble();

        info.resolution = result.value("resolution").toDouble();

        info.mode = result.value("mode").toString();

        info.layerCount = result.value("layerCount").toInt();

        emit imageInspected(info);

        return;
    }
}
void AdobePhotoshopBridge::handlePingTimeout()
{
    if (m_pendingPingId.isEmpty())
    {
        m_inspectionTimeout.stop();

        m_pendingInspectionId.clear();

        emit imageInspectionFailed(tr("Se perdió la conexión con Adobe Photoshop."));
    }

    m_pendingPingId.clear();

    emit pingFailed(tr("Adobe Photoshop no respondió a la solicitud."));
}

void AdobePhotoshopBridge::inspectImage(const QString &path)
{
    if (!m_transport)
    {
        emit imageInspectionFailed(tr("Adobe Bridge no está disponible."));

        return;
    }

    if (!m_transport->hasClient(AdobeHost::Photoshop))
    {
        emit imageInspectionFailed(tr("Adobe Photoshop no está conectado."));

        return;
    }

    if (!m_pendingInspectionId.isEmpty())
    {
        emit imageInspectionFailed(tr("Ya hay una inspección de imagen pendiente."));

        return;
    }

    if (path.isEmpty())
    {
        emit imageInspectionFailed(tr("La ruta del archivo está vacía."));

        return;
    }

    m_pendingInspectionId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    QJsonObject request;

    request["version"] = 1;

    request["id"] = m_pendingInspectionId;

    request["command"] = QStringLiteral("inspectImage");

    request["path"] = path;

    const QString message = QString::fromUtf8(QJsonDocument(request).toJson(QJsonDocument::Compact));

    m_transport->sendTextMessage(AdobeHost::Photoshop, message);

    m_inspectionTimeout.start();
}
