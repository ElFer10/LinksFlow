#include "adobebridgetransport.h"

#include <QHostAddress>
#include <QWebSocket>

AdobeBridgeTransport::AdobeBridgeTransport(QObject *parent)
    : QObject(parent), m_server(QStringLiteral("LinksFlow Adobe Bridge"),
                                QWebSocketServer::NonSecureMode, this) {
  connect(&m_server, &QWebSocketServer::newConnection, this, [this]() {
    if (m_client) {
      QWebSocket *extraClient = m_server.nextPendingConnection();

      if (extraClient) {
        extraClient->close();
        extraClient->deleteLater();
      }

      return;
    }

    m_client = m_server.nextPendingConnection();

    if (!m_client) {
      return;
    }

    connect(m_client, &QWebSocket::textMessageReceived, this,
            &AdobeBridgeTransport::textMessageReceived);

    connect(m_client, &QWebSocket::disconnected, this, [this]() {
      if (!m_client) {
        return;
      }

      m_client->deleteLater();
      m_client = nullptr;

      emit clientDisconnected();
    });

    emit clientConnected();
  });
}

bool AdobeBridgeTransport::start(quint16 port) {
  if (m_server.isListening()) {
    return true;
  }

  return m_server.listen(QHostAddress::LocalHost, port);
}

void AdobeBridgeTransport::stop() {
  if (m_client) {
    m_client->close();
  }

  m_server.close();
}

bool AdobeBridgeTransport::isListening() const {
  return m_server.isListening();
}
