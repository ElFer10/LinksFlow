#pragma once

#include <QObject>
#include <QWebSocketServer>

class QWebSocket;

class AdobeBridgeTransport : public QObject {
  Q_OBJECT

public:
  explicit AdobeBridgeTransport(QObject *parent = nullptr);

  bool start(quint16 port = 17321);
  void stop();
  bool isListening() const;
  void sendTextMessage(const QString &message);
  bool hasClient() const;

signals:
  void clientConnected();
  void clientDisconnected();
  void textMessageReceived(const QString &message);
  void connectionChanged(bool connected);

private:
  QWebSocketServer m_server;

  QWebSocket *m_client = nullptr;
};
