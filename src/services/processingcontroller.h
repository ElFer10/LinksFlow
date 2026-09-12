#pragma once

#include "../domain/linkinfo.h"
#include "../domain/optimizationsettings.h"
#include "../domain/processingjob.h"
#include "../domain/processingresult.h"

#include <QList>
#include <QObject>

class AdobePhotoshopBridge;
class InDesignBridge;
struct PhotoshopProcessResult;

class ProcessingController : public QObject
{
    Q_OBJECT

  public:
    explicit ProcessingController(AdobePhotoshopBridge *photoshopBridge, InDesignBridge *indesignBridge,
                                  QObject *parent = nullptr);
    QList<ProcessingJob> createJobs(const QList<LinkInfo> &links, const OptimizationSettings &settings) const;

    void processJobs(const QList<ProcessingJob> &jobs);

    void cancelProcessing();

  signals:
    void processingStarted(int totalJobs);
    void jobStarted(const ProcessingJob &job);
    void jobCompleted(const ProcessingResult &result);
    void processingCompleted();
    void processingFailed(const QString &message);

  private:
    void processNextJob();

    void handleResolutionProcessed(const PhotoshopProcessResult &result);
    void handleResolutionProcessingFailed(const QString &message);
    void finishCurrentJob(ProcessingJobState state, const QString &message);
    double scaleFactorForJob(const ProcessingJob &job) const;
    void handleLinksUpdated(int updatedCount);
    void handleLinksUpdateFailed(const QString &message);
    void completeCurrentJob();
    InDesignBridge *m_indesignBridge = nullptr;

  private:
    AdobePhotoshopBridge *m_photoshopBridge = nullptr;

    QList<ProcessingJob> m_jobs;

    int m_currentJobIndex = -1;

    bool m_cancelRequested = false;
    bool m_processing = false;

    qint64 m_currentOriginalSizeBytes = 0;
};
