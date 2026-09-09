#pragma once

#include <QObject>
#include <QTimer>

class AdobeBridgeTransport;

class AdobePhotoshopBridge : public QObject
{
    Q_OBJECT

  public:
    explicit AdobePhotoshopBridge(AdobeBridgeTransport *transport, QObject *parent = nullptr);

    bool isConnected() const;

    void ping();

  signals:
    void pingSucceeded();
    void pingFailed(const QString &message);

  private:
    void handleMessage(const QString &message);

    void handlePingTimeout();

  private:
    AdobeBridgeTransport *m_transport = nullptr;

    QString m_pendingPingId;

    QTimer m_pingTimeout;
};
