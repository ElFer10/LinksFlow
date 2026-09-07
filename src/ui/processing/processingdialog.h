#pragma once

#include "../../domain/processingjob.h"
#include "../../domain/processingresult.h"

#include <QDialog>
#include <QList>

class QLabel;
class QPushButton;
class QProgressBar;
class QTableView;

class ProcessingTableModel;

class ProcessingDialog : public QDialog
{
    Q_OBJECT

  public:
    explicit ProcessingDialog(QWidget *parent = nullptr);

    void setJobs(const QList<ProcessingJob> &jobs);

    void jobStarted(const ProcessingJob &job);

    void jobCompleted(const ProcessingResult &result);

    void processingCompleted();

  signals:
    void cancelRequested();

  private:
    void setupUi();
    void setupConnections();
    void updateSummary();

  private:
    ProcessingTableModel *m_model = nullptr;

    QTableView *m_table = nullptr;

    QProgressBar *m_progressBar = nullptr;

    QLabel *m_progressLabel = nullptr;

    QLabel *m_summaryLabel = nullptr;

    QPushButton *m_cancelButton = nullptr;

    QPushButton *m_closeButton = nullptr;

    int m_totalJobs = 0;
    int m_finishedJobs = 0;
};
