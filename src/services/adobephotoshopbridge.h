#pragma once

#include <QObject>
#include <QTimer>

class AdobeBridgeTransport;

struct PhotoshopImageInfo
{
    QString name;
    QString path;

    double width = 0.0;
    double height = 0.0;
    double resolution = 0.0;

    QString mode;

    int layerCount = 0;
};

class AdobePhotoshopBridge : public QObject
{
    Q_OBJECT

  public:
    explicit AdobePhotoshopBridge(AdobeBridgeTransport *transport, QObject *parent = nullptr);

    bool isConnected() const;
    void ping();

    void inspectImage(const QString &path);

  signals:
    void pingSucceeded();
    void pingFailed(const QString &message);
    void imageInspected(const PhotoshopImageInfo &info);
    void imageInspectionFailed(const QString &message);

  private:
    void handleMessage(const QString &message);
    void handlePingTimeout();
    void handleInspectionTimeout();

  private:
    AdobeBridgeTransport *m_transport = nullptr;
    QString m_pendingInspectionId;
    QTimer m_inspectionTimeout;

    QString m_pendingPingId;
    QTimer m_pingTimeout;
};
