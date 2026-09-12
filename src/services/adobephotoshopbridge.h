#pragma once

#include <QObject>
#include <QTimer>

class AdobeBridgeTransport;

struct PhotoshopProcessResult
{
    QString sourcePath;
    QString outputPath;

    double originalWidth = 0.0;
    double originalHeight = 0.0;
    double originalResolution = 0.0;

    double processedWidth = 0.0;
    double processedHeight = 0.0;
    double processedResolution = 0.0;
};

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
    void processResolution(const QString &sourcePath, double scaleFactor);

  signals:
    void pingSucceeded();
    void pingFailed(const QString &message);
    void imageInspected(const PhotoshopImageInfo &info);
    void imageInspectionFailed(const QString &message);
    void resolutionProcessed(const PhotoshopProcessResult &result);
    void resolutionProcessingFailed(const QString &message);

  private:
    void handleMessage(const QString &message);
    void handlePingTimeout();
    void handleInspectionTimeout();
    void handleProcessingTimeout();

  private:
    AdobeBridgeTransport *m_transport = nullptr;
    QString m_pendingInspectionId;
    QTimer m_inspectionTimeout;

    QString m_pendingPingId;
    QTimer m_pingTimeout;

    QString m_pendingProcessingId;
    QTimer m_processingTimeout;
};
