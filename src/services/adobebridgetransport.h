#pragma once

#include <QObject>
#include <QWebSocketServer>

class QWebSocket;

enum class AdobeHost
{
    Unknown,
    InDesign,
    Photoshop
};

class AdobeBridgeTransport : public QObject
{
    Q_OBJECT

  public:
    explicit AdobeBridgeTransport(QObject *parent = nullptr);

    bool start(quint16 port = 17321);
    void stop();

    bool isListening() const;

    bool hasClient(AdobeHost host) const;

    void sendTextMessage(AdobeHost host, const QString &message);

  signals:
    void clientConnected(AdobeHost host);

    void clientDisconnected(AdobeHost host);

    void textMessageReceived(AdobeHost host, const QString &message);

    void connectionChanged(AdobeHost host, bool connected);

  private:
    void handleNewConnection();

    void handleInitialMessage(QWebSocket *socket, const QString &message);

    AdobeHost hostFromString(const QString &value) const;

    QWebSocket *socketForHost(AdobeHost host) const;

  private:
    QWebSocketServer m_server;

    QWebSocket *m_indesignClient = nullptr;
    QWebSocket *m_photoshopClient = nullptr;

    bool m_stopping = false;
};
