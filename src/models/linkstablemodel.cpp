#include "linkstablemodel.h"

#include <QtMath>

LinksTableModel::LinksTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int LinksTableModel::rowCount(
    const QModelIndex &parent
    ) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_links.size();
}

int LinksTableModel::columnCount(
    const QModelIndex &parent
    ) const
{
    if (parent.isValid()) {
        return 0;
    }

    return ColumnCount;
}

QVariant LinksTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    if (index.row() < 0 || index.row() >= m_links.size()) {
        return {};
    }

    const LinkInfo &link = m_links.at(index.row());
    const bool hasUnavailableMetadata = link.state == LinkProcessState::Missing || link.state == LinkProcessState::Error;

    if (index.column() == ProcessColumn && role == Qt::CheckStateRole) {
        return link.process ? Qt::Checked : Qt::Unchecked;
    }

    if (role != Qt::DisplayRole) {
        return {};
    }

    switch (index.column()) {
    case ProcessColumn:
        return {};

    case PageColumn:
        return link.page;

    case NameColumn:
        return link.fileName;

    case TypeColumn:
        return link.fileType;

    case ColorColumn:
        return link.colorMode;

    case SizeColumn:
      if (hasUnavailableMetadata) {
        return QStringLiteral("—");
      }

      return formatFileSize(link.fileSizeBytes);

   case EffectiveResolutionColumn:
    if (hasUnavailableMetadata) {
        return QStringLiteral("—");
    }

    return formatResolution(
        link.effectiveResolution
    );

    case IccProfileColumn:
        return link.iccProfile;

    default:
        return {};
    }
}

bool LinksTableModel::setData(
    const QModelIndex &index,
    const QVariant &value,
    int role
    )
{
    if (!index.isValid()) {
        return false;
    }

    if (
        index.row() < 0 ||
        index.row() >= m_links.size()
        ) {
        return false;
    }

    if ( index.column() != ProcessColumn || role != Qt::CheckStateRole ) {
        return false;
    }

    LinkInfo &link = m_links[index.row()];

    const bool checked =
        value.toInt() == Qt::Checked;

    if (link.process == checked) {
        return false;
    }

    link.process = checked;

    emit dataChanged(index, index, {Qt::CheckStateRole});
    emit processSelectionChanged();

    return true;
}

QVariant LinksTableModel::headerData(
    int section,
    Qt::Orientation orientation,
    int role
    ) const
{
    if (
        orientation != Qt::Horizontal ||
        role != Qt::DisplayRole
        ) {
        return {};
    }

    switch (section) {
    case ProcessColumn:
        return {};

    case PageColumn:
        return tr("Página");

    case NameColumn:
        return tr("Nombre");

    case TypeColumn:
        return tr("Tipo");

    case ColorColumn:
        return tr("Color");

    case SizeColumn:
        return tr("Tamaño");

    case EffectiveResolutionColumn:
        return tr("Resolución efectiva");

    case IccProfileColumn:
        return tr("Perfil ICC");

    default:
        return {};
    }
}

Qt::ItemFlags LinksTableModel::flags(
    const QModelIndex &index
    ) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    Qt::ItemFlags result =
        Qt::ItemIsEnabled |
        Qt::ItemIsSelectable;

    if (
        index.column() ==
        ProcessColumn
        ) {
        const LinkInfo &link =
            m_links.at(index.row());

        if (
            link.state ==
            LinkProcessState::Ready
            ) {
            result |=
                Qt::ItemIsUserCheckable;
        }
    }

    return result;
}

void LinksTableModel::setLinks(
    const QList<LinkInfo> &links
    )
{
    beginResetModel();

    m_links = links;

    endResetModel();
}

QList<LinkInfo> LinksTableModel::links() const
{
    return m_links;
}

LinkInfo LinksTableModel::linkAt(
    int row
    ) const
{
    if (
        row < 0 ||
        row >= m_links.size()
        ) {
        return {};
    }

    return m_links.at(row);
}

QString LinksTableModel::formatFileSize(
    qint64 bytes
    ) const
{
    if (bytes < 1024) {
        return tr("%1 B")
        .arg(bytes);
    }

    const double kilobytes =
        bytes / 1024.0;

    if (kilobytes < 1024.0) {
        return tr("%1 KB")
        .arg(
            kilobytes,
            0,
            'f',
            1
            );
    }

    const double megabytes =
        kilobytes / 1024.0;

    if (megabytes < 1024.0) {
        return tr("%1 MB")
        .arg(
            megabytes,
            0,
            'f',
            1
            );
    }

    const double gigabytes =
        megabytes / 1024.0;

    return tr("%1 GB")
        .arg(
            gigabytes,
            0,
            'f',
            2
            );
}

QString LinksTableModel::formatResolution(
    const Resolution2D &resolution
    ) const
{
    if (resolutionsAreEqual(resolution)) {
        return tr("%1 ppi")
        .arg(
            qRound(resolution.x)
            );
    }

    return tr("%1 × %2 ppi")
        .arg(
            qRound(resolution.x)
            )
        .arg(
            qRound(resolution.y)
            );
}

bool LinksTableModel::resolutionsAreEqual(
    const Resolution2D &resolution
    ) const
{
    constexpr double tolerance = 0.5;

    return qAbs(
               resolution.x -
               resolution.y
               ) < tolerance;
}

int LinksTableModel::processableCount() const
{
    int count = 0;

    for (const LinkInfo &link : m_links) {
        if (link.state == LinkProcessState::Ready) {
            ++count;
        }
    }

    return count;
}

int LinksTableModel::selectedForProcessingCount() const
{
    int count = 0;

    for (const LinkInfo &link : m_links) {
        if (
            link.state == LinkProcessState::Ready &&
            link.process
        ) {
            ++count;
        }
    }

    return count;
}
