#include "processingdialog.h"

#include "../../models/processingtablemodel.h"

#include <QAbstractItemView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

ProcessingDialog::ProcessingDialog(QWidget *parent) : QDialog(parent)
{
    setupUi();
    setupConnections();
}

void ProcessingDialog::setupUi()
{
    setWindowTitle(tr("Procesando imágenes"));

    setModal(true);

    resize(900, 500);

    auto *mainLayout = new QVBoxLayout(this);

    //
    // Progreso
    //

    m_progressLabel = new QLabel(tr("Preparando procesamiento..."), this);

    mainLayout->addWidget(m_progressLabel);

    m_progressBar = new QProgressBar(this);

    m_progressBar->setRange(0, 0);

    mainLayout->addWidget(m_progressBar);

    //
    // Tabla
    //

    m_model = new ProcessingTableModel(this);

    m_table = new QTableView(this);

    m_table->setModel(m_model);

    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_table->setSelectionMode(QAbstractItemView::SingleSelection);

    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_table->setAlternatingRowColors(true);

    m_table->verticalHeader()->setVisible(false);

    QHeaderView *header = m_table->horizontalHeader();

    header->setSectionResizeMode(ProcessingTableModel::FileNameColumn, QHeaderView::Stretch);

    header->setSectionResizeMode(ProcessingTableModel::ActionColumn, QHeaderView::Stretch);

    header->setSectionResizeMode(ProcessingTableModel::StateColumn, QHeaderView::ResizeToContents);

    header->setSectionResizeMode(ProcessingTableModel::OriginalSizeColumn, QHeaderView::ResizeToContents);

    header->setSectionResizeMode(ProcessingTableModel::ProcessedSizeColumn, QHeaderView::ResizeToContents);

    header->setSectionResizeMode(ProcessingTableModel::OptimizationColumn, QHeaderView::ResizeToContents);

    mainLayout->addWidget(m_table, 1);

    //
    // Resumen
    //

    m_summaryLabel = new QLabel(this);

    mainLayout->addWidget(m_summaryLabel);

    //
    // Botones
    //

    auto *buttonLayout = new QHBoxLayout;

    buttonLayout->addStretch();

    m_cancelButton = new QPushButton(tr("Cancelar"), this);

    m_closeButton = new QPushButton(tr("Cerrar"), this);

    m_closeButton->setVisible(false);

    buttonLayout->addWidget(m_cancelButton);

    buttonLayout->addWidget(m_closeButton);

    mainLayout->addLayout(buttonLayout);

    updateSummary();
}

void ProcessingDialog::setupConnections()
{
    connect(m_cancelButton, &QPushButton::clicked, this, &ProcessingDialog::cancelRequested);

    connect(m_closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

void ProcessingDialog::setJobs(const QList<ProcessingJob> &jobs)
{
    m_totalJobs = jobs.size();

    m_finishedJobs = 0;

    m_model->setJobs(jobs);

    m_progressBar->setRange(0, m_totalJobs);

    m_progressBar->setValue(0);

    m_progressLabel->setText(tr("0 de %1 imágenes procesadas").arg(m_totalJobs));

    m_cancelButton->setVisible(true);
    m_closeButton->setVisible(false);

    updateSummary();
}

void ProcessingDialog::jobStarted(const ProcessingJob &job)
{
    m_model->setJobState(job.id, ProcessingJobState::Processing);

    m_progressLabel->setText(tr("Procesando %1 de %2...").arg(m_finishedJobs + 1).arg(m_totalJobs));
}

void ProcessingDialog::jobCompleted(const ProcessingResult &result)
{
    m_model->applyResult(result);

    ++m_finishedJobs;

    m_progressBar->setValue(m_finishedJobs);

    m_progressLabel->setText(tr("%1 de %2 imágenes procesadas").arg(m_finishedJobs).arg(m_totalJobs));

    updateSummary();
}

void ProcessingDialog::processingCompleted()
{
    m_progressBar->setValue(m_totalJobs);

    m_progressLabel->setText(tr("Procesamiento finalizado"));

    m_cancelButton->setVisible(false);
    m_closeButton->setVisible(true);

    m_closeButton->setDefault(true);
    m_closeButton->setFocus();

    updateSummary();
}

void ProcessingDialog::updateSummary()
{
    m_summaryLabel->setText(tr("Completados: %1   "
                               "Omitidos: %2   "
                               "Errores: %3")
                                .arg(m_model->completedCount())
                                .arg(m_model->skippedCount())
                                .arg(m_model->failedCount()));
}
