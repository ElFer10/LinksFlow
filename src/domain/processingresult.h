#pragma once

#include "processingjob.h"

#include <QString>

struct ProcessingResult
{
    QString jobId;

    ProcessingJobState state = ProcessingJobState::Pending;

    QString sourcePath;
    QString outputPath;

    qint64 originalSizeBytes = 0;
    qint64 processedSizeBytes = 0;

    QString message;

    bool success() const
    {
        return state == ProcessingJobState::Completed;
    }
};
