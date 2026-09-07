#pragma once

#include "../domain/processingjob.h"
#include "../domain/processingresult.h"

#include <QAbstractTableModel>
#include <QList>

class ProcessingTableModel : public QAbstractTableModel
{
    Q_OBJECT

  public:
    enum Column
    {
        FileNameColumn = 0,
        ActionColumn,
        StateColumn,
        OriginalSizeColumn,
        ProcessedSizeColumn,
        OptimizationColumn,
        ColumnCount
    };

    explicit ProcessingTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void setJobs(const QList<ProcessingJob> &jobs);

    void setJobState(const QString &jobId, ProcessingJobState state, const QString &message = QString());

    void applyResult(const ProcessingResult &result);

    const ProcessingJob *jobAt(int row) const;

    int completedCount() const;
    int failedCount() const;
    int skippedCount() const;

  private:
    int findJobRow(const QString &jobId) const;

    QString actionText(const ProcessingJob &job) const;

    QString stateText(ProcessingJobState state) const;

    QString formatFileSize(qint64 bytes) const;

    QString optimizationText(const ProcessingJob &job) const;

  private:
    struct RowData
    {
        ProcessingJob job;

        qint64 originalSizeBytes = 0;
        qint64 processedSizeBytes = 0;
    };

    QList<RowData> m_rows;
};
