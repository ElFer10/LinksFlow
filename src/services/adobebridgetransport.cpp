#include "adobebridgetransport.h"

#include <QAbstractSocket>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QWebSocket>

AdobeBridgeTransport::AdobeBridgeTransport(QObject *parent)
    : QObject(parent), m_server(QStringLiteral("LinksFlow Adobe Bridge"), QWebSocketServer::NonSecureMode, this)
{
    connect(&m_server, &QWebSocketServer::newConnection, this, &AdobeBridgeTransport::handleNewConnection);
}

bool AdobeBridgeTransport::start(quint16 port)
{
    if (m_server.isListening())
    {
        return true;
    }

    m_stopping = false;

    return m_server.listen(QHostAddress::LocalHost, port);
}

void AdobeBridgeTransport::stop()
{
    m_stopping = true;

    if (m_indesignClient)
    {
        disconnect(m_indesignClient, nullptr, this, nullptr);

        m_indesignClient->close();
        m_indesignClient->deleteLater();
        m_indesignClient = nullptr;
    }

    if (m_photoshopClient)
    {
        disconnect(m_photoshopClient, nullptr, this, nullptr);

        m_photoshopClient->close();
        m_photoshopClient->deleteLater();
        m_photoshopClient = nullptr;
    }

    m_server.close();
}

bool AdobeBridgeTransport::isListening() const
{
    return m_server.isListening();
}

bool AdobeBridgeTransport::hasClient(AdobeHost host) const
{
    QWebSocket *socket = socketForHost(host);

    return socket && socket->state() == QAbstractSocket::ConnectedState;
}

void AdobeBridgeTransport::sendTextMessage(AdobeHost host, const QString &message)
{
    QWebSocket *socket = socketForHost(host);

    if (!socket)
    {
        return;
    }

    if (socket->state() != QAbstractSocket::ConnectedState)
    {
        return;
    }

    socket->sendTextMessage(message);
}

void AdobeBridgeTransport::handleNewConnection()
{
    QWebSocket *socket = m_server.nextPendingConnection();

    if (!socket)
    {
        return;
    }

    //
    // Todavía no sabemos si este socket pertenece
    // a InDesign o Photoshop.
    //
    // El primer mensaje "bridgeReady" lo identificará.
    //

    connect(socket, &QWebSocket::textMessageReceived, this,
            [this, socket](const QString &message) { handleInitialMessage(socket, message); });

    connect(socket, &QWebSocket::disconnected, this, [this, socket]() {
        AdobeHost host = AdobeHost::Unknown;

        if (socket == m_indesignClient)
        {
            host = AdobeHost::InDesign;

            m_indesignClient = nullptr;
        }
        else if (socket == m_photoshopClient)
        {
            host = AdobeHost::Photoshop;

            m_photoshopClient = nullptr;
        }

        socket->deleteLater();

        if (!m_stopping && host != AdobeHost::Unknown)
        {
            emit clientDisconnected(host);

            emit connectionChanged(host, false);
        }
    });
}

void AdobeBridgeTransport::handleInitialMessage(QWebSocket *socket, const QString &message)
{
    if (!socket)
    {
        return;
    }

    //
    // Si el socket ya está identificado,
    // simplemente reenviamos el mensaje.
    //

    if (socket == m_indesignClient)
    {
        emit textMessageReceived(AdobeHost::InDesign, message);

        return;
    }

    if (socket == m_photoshopClient)
    {
        emit textMessageReceived(AdobeHost::Photoshop, message);

        return;
    }

    //
    // Socket nuevo: esperamos bridgeReady.
    //

    QJsonParseError parseError;

    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError || !document.isObject())
    {
        return;
    }

    const QJsonObject object = document.object();

    if (object.value("event").toString() != QStringLiteral("bridgeReady"))
    {
        return;
    }

    const AdobeHost host = hostFromString(object.value("host").toString());
    if (host == AdobeHost::Unknown)
    {
        qWarning() << "Adobe Bridge: host desconocido:" << object.value("host").toString() << "Mensaje:" << message;

        return;
    }

    if (host == AdobeHost::Unknown)
    {
        return;
    }

    //
    // Registramos el socket.
    //

    if (host == AdobeHost::InDesign)
    {

        if (m_indesignClient && m_indesignClient != socket)
        {
            m_indesignClient->close();
        }

        m_indesignClient = socket;
    }
    else if (host == AdobeHost::Photoshop)
    {

        if (m_photoshopClient && m_photoshopClient != socket)
        {
            m_photoshopClient->close();
        }

        m_photoshopClient = socket;
    }

    emit clientConnected(host);

    emit connectionChanged(host, true);

    //
    // bridgeReady también puede interesarle
    // a la capa superior.
    //

    emit textMessageReceived(host, message);
}

AdobeHost AdobeBridgeTransport::hostFromString(const QString &value) const
{
    const QString normalized = value.trimmed().toLower();

    if (normalized == QStringLiteral("indesign"))
    {
        return AdobeHost::InDesign;
    }

    if (normalized == QStringLiteral("photoshop"))
    {
        return AdobeHost::Photoshop;
    }

    return AdobeHost::Unknown;
}

QWebSocket *AdobeBridgeTransport::socketForHost(AdobeHost host) const
{
    switch (host)
    {

    case AdobeHost::InDesign:
        return m_indesignClient;

    case AdobeHost::Photoshop:
        return m_photoshopClient;

    case AdobeHost::Unknown:
        return nullptr;
    }

    return nullptr;
}
