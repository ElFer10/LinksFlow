#include "processingtablemodel.h"

#include <QFileInfo>
#include <QStringList>

ProcessingTableModel::ProcessingTableModel(QObject *parent) : QAbstractTableModel(parent)
{
}

int ProcessingTableModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        return 0;
    }

    return m_rows.size();
}

int ProcessingTableModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
    {
        return 0;
    }

    return ColumnCount;
}

QVariant ProcessingTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
    {
        return {};
    }

    const RowData &row = m_rows.at(index.row());

    const ProcessingJob &job = row.job;

    if (role == Qt::DisplayRole)
    {

        switch (index.column())
        {

        case FileNameColumn:
            return QFileInfo(job.sourcePath).fileName();

        case ActionColumn:
            return actionText(job);

        case StateColumn:
            if (!job.statusMessage.isEmpty())
            {
                return QStringLiteral("%1 — %2").arg(stateText(job.state), job.statusMessage);
            }

            return stateText(job.state);

        case OriginalSizeColumn:
            if (row.originalSizeBytes <= 0)
            {
                return QStringLiteral("—");
            }

            return formatFileSize(row.originalSizeBytes);

        case ProcessedSizeColumn:
            if (row.processedSizeBytes <= 0)
            {
                return QStringLiteral("—");
            }

            return formatFileSize(row.processedSizeBytes);

        case OptimizationColumn:
            return optimizationText(job);

        default:
            break;
        }
    }

    if (role == Qt::ToolTipRole)
    {

        if (index.column() == FileNameColumn)
        {
            return job.sourcePath;
        }

        if (index.column() == ActionColumn)
        {
            return actionText(job);
        }

        if (index.column() == StateColumn)
        {
            return job.statusMessage;
        }
    }

    return {};
}

QVariant ProcessingTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
    {
        return {};
    }

    switch (section)
    {

    case FileNameColumn:
        return tr("Archivo");

    case ActionColumn:
        return tr("Acción");

    case StateColumn:
        return tr("Estado");

    case OriginalSizeColumn:
        return tr("Original");

    case ProcessedSizeColumn:
        return tr("Final");

    case OptimizationColumn:
        return tr("Optimización");

    default:
        return {};
    }
}

void ProcessingTableModel::setJobs(const QList<ProcessingJob> &jobs)
{
    beginResetModel();

    m_rows.clear();
    m_rows.reserve(jobs.size());

    for (const ProcessingJob &job : jobs)
    {

        RowData row;
        row.job = job;

        const QFileInfo fileInfo(job.sourcePath);

        if (fileInfo.exists() && fileInfo.isFile())
        {
            row.originalSizeBytes = fileInfo.size();
        }

        m_rows.append(row);
    }

    endResetModel();
}

void ProcessingTableModel::setJobState(const QString &jobId, ProcessingJobState state, const QString &message)
{
    const int row = findJobRow(jobId);

    if (row < 0)
    {
        return;
    }

    m_rows[row].job.state = state;

    m_rows[row].job.statusMessage = message;

    emit dataChanged(index(row, StateColumn), index(row, StateColumn), {Qt::DisplayRole, Qt::ToolTipRole});
}

void ProcessingTableModel::applyResult(const ProcessingResult &result)
{
    const int row = findJobRow(result.jobId);

    if (row < 0)
    {
        return;
    }

    RowData &rowData = m_rows[row];

    rowData.job.state = result.state;

    rowData.job.statusMessage = result.message;

    rowData.originalSizeBytes = result.originalSizeBytes;

    rowData.processedSizeBytes = result.processedSizeBytes;

    emit dataChanged(index(row, StateColumn), index(row, OptimizationColumn), {Qt::DisplayRole, Qt::ToolTipRole});
}

const ProcessingJob *ProcessingTableModel::jobAt(int row) const
{
    if (row < 0 || row >= m_rows.size())
    {
        return nullptr;
    }

    return &m_rows.at(row).job;
}

int ProcessingTableModel::completedCount() const
{
    int count = 0;

    for (const RowData &row : m_rows)
    {
        if (row.job.state == ProcessingJobState::Completed)
        {
            ++count;
        }
    }

    return count;
}

int ProcessingTableModel::failedCount() const
{
    int count = 0;

    for (const RowData &row : m_rows)
    {
        if (row.job.state == ProcessingJobState::Failed)
        {
            ++count;
        }
    }

    return count;
}

int ProcessingTableModel::skippedCount() const
{
    int count = 0;

    for (const RowData &row : m_rows)
    {
        if (row.job.state == ProcessingJobState::Skipped)
        {
            ++count;
        }
    }

    return count;
}

int ProcessingTableModel::findJobRow(const QString &jobId) const
{
    for (int row = 0; row < m_rows.size(); ++row)
    {
        if (m_rows.at(row).job.id == jobId)
        {
            return row;
        }
    }

    return -1;
}

QString ProcessingTableModel::actionText(const ProcessingJob &job) const
{
    QStringList actions;

    if (job.resizeRequired)
    {
        actions.append(tr("Resolución"));
    }

    if (job.colorModeConversionRequired)
    {
        actions.append(tr("Modo de color"));
    }

    if (job.colorProfileConversionRequired)
    {
        actions.append(tr("Perfil ICC"));
    }

    if (job.removeHiddenLayers)
    {
        actions.append(tr("Eliminar capas ocultas"));
    }

    if (job.mergeVisibleLayers)
    {
        actions.append(tr("Combinar capas"));
    }

    if (job.flattenImage)
    {
        actions.append(tr("Acoplar"));
    }

    if (job.alphaChannels == AlphaChannelHandling::Remove)
    {
        actions.append(tr("Eliminar canales alfa"));
    }

    if (job.formatConversionRequired)
    {
        actions.append(tr("Convertir formato"));
    }

    if (actions.isEmpty())
    {
        return tr("Sin cambios");
    }

    return actions.join(QStringLiteral(", "));
}

QString ProcessingTableModel::stateText(ProcessingJobState state) const
{
    switch (state)
    {

    case ProcessingJobState::Pending:
        return tr("Pendiente");

    case ProcessingJobState::Processing:
        return tr("Procesando");

    case ProcessingJobState::Completed:
        return tr("Completado");

    case ProcessingJobState::Skipped:
        return tr("Omitido");

    case ProcessingJobState::Failed:
        return tr("Error");
    }

    return {};
}

QString ProcessingTableModel::formatFileSize(qint64 bytes) const
{
    constexpr double kilo = 1024.0;

    constexpr double mega = 1024.0 * 1024.0;

    constexpr double giga = 1024.0 * 1024.0 * 1024.0;

    if (bytes >= giga)
    {
        return QStringLiteral("%1 GB").arg(bytes / giga, 0, 'f', 2);
    }

    if (bytes >= mega)
    {
        return QStringLiteral("%1 MB").arg(bytes / mega, 0, 'f', 1);
    }

    if (bytes >= kilo)
    {
        return QStringLiteral("%1 KB").arg(bytes / kilo, 0, 'f', 1);
    }

    return QStringLiteral("%1 B").arg(bytes);
}

QString ProcessingTableModel::optimizationText(const ProcessingJob &job) const
{
    const int row = findJobRow(job.id);

    if (row < 0)
    {
        return QStringLiteral("—");
    }

    const RowData &rowData = m_rows.at(row);

    if (rowData.originalSizeBytes <= 0 || rowData.processedSizeBytes <= 0)
    {
        return QStringLiteral("—");
    }

    if (rowData.originalSizeBytes == 0)
    {
        return QStringLiteral("—");
    }

    const double reduction = 100.0 * (1.0 - static_cast<double>(rowData.processedSizeBytes) /
                                                static_cast<double>(rowData.originalSizeBytes));

    if (reduction > 0.0)
    {
        return QStringLiteral("%1 %").arg(reduction, 0, 'f', 1);
    }

    if (reduction < 0.0)
    {
        return QStringLiteral("+%1 %").arg(-reduction, 0, 'f', 1);
    }

    return QStringLiteral("0 %");
}
