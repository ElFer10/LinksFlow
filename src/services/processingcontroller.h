#pragma once

#include "../domain/linkinfo.h"
#include "../domain/optimizationsettings.h"
#include "../domain/processingjob.h"
#include "../domain/processingresult.h"

#include <QList>
#include <QObject>

class ProcessingController : public QObject
{
    Q_OBJECT

  public:
    explicit ProcessingController(QObject *parent = nullptr);
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

    QList<ProcessingJob> m_jobs;
    int m_currentJobIndex = -1;
    bool m_cancelRequested = false;
};
