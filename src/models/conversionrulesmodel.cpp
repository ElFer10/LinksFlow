#include "conversionrulesmodel.h"

ConversionRulesModel::ConversionRulesModel(
    QObject *parent
)
    : QAbstractTableModel(parent)
{
    createDefaultRules();
}

int ConversionRulesModel::rowCount(
    const QModelIndex &parent
) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_rules.size();
}

int ConversionRulesModel::columnCount(
    const QModelIndex &parent
) const
{
    if (parent.isValid()) {
        return 0;
    }

    return ColumnCount;
}

QVariant ConversionRulesModel::data(
    const QModelIndex &index,
    int role
) const
{
    if (
        !index.isValid() ||
        index.row() < 0 ||
        index.row() >= m_rules.size()
    ) {
        return {};
    }

    const ConversionRule &rule =
        m_rules.at(index.row());

    switch (index.column()) {

    case EnabledColumn:
        if (role == Qt::CheckStateRole) {
            return rule.enabled
                ? Qt::Checked
                : Qt::Unchecked;
        }

        break;

    case SourceFormatColumn:
        if (role == Qt::DisplayRole) {
            return formatText(
                rule.sourceFormat
            );
        }

        break;

    case DestinationFormatColumn:
        if (
            role == Qt::DisplayRole ||
            role == Qt::EditRole
        ) {
            return formatText(
                rule.destinationFormat
            );
        }

        break;

    case OptionsColumn:
        if (role == Qt::DisplayRole) {
            return tr("Opciones...");
        }

        break;
    }

    return {};
}

bool ConversionRulesModel::setData(
    const QModelIndex &index,
    const QVariant &value,
    int role
)
{
    if (
        !index.isValid() ||
        index.row() < 0 ||
        index.row() >= m_rules.size()
    ) {
        return false;
    }

    ConversionRule &rule =
        m_rules[index.row()];

    if (
        index.column() == EnabledColumn &&
        role == Qt::CheckStateRole
    ) {
        rule.enabled =
            value.toInt() == Qt::Checked;

        emit dataChanged(
            index,
            index,
            {Qt::CheckStateRole}
        );

        emit settingsChanged();

        return true;
    }

    if ( index.column() == DestinationFormatColumn && role == Qt::EditRole)
    {

      const ImageFormat newFormat = static_cast<ImageFormat>(value.toInt());

    if (newFormat == rule.sourceFormat) {
        return false;
    }

    rule.destinationFormat =
        newFormat;

    emit dataChanged(
        index,
        index,
        {
            Qt::DisplayRole,
            Qt::EditRole
        }
    );

    emit settingsChanged();

    return true;
}

    return false;
}

QVariant ConversionRulesModel::headerData(
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
    case EnabledColumn:
        return QString();

    case SourceFormatColumn:
        return tr("Formato original");

    case DestinationFormatColumn:
        return tr("Convertir a");

    case OptionsColumn:
        return tr("Opciones");

    default:
        return {};
    }
}

Qt::ItemFlags ConversionRulesModel::flags(
    const QModelIndex &index
) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    Qt::ItemFlags result =
        Qt::ItemIsEnabled |
        Qt::ItemIsSelectable;

    if (!m_conversionEnabled) {
        return Qt::ItemIsSelectable;
    }

    if (index.column() == EnabledColumn) {
        result |= Qt::ItemIsUserCheckable;
    }

    if (index.column() == DestinationFormatColumn) {
        result |= Qt::ItemIsEditable;
    }

    return result;
}

void ConversionRulesModel::createDefaultRules()
{
    m_rules.clear();

    const QList<ImageFormat> formats = {
        ImageFormat::PSD,
        ImageFormat::TIFF,
        ImageFormat::JPEG,
        ImageFormat::PNG,
        ImageFormat::WebP,
        ImageFormat::BMP
    };

    for (ImageFormat format : formats) {
        ConversionRule rule;

        rule.sourceFormat = format;
        rule.enabled = false;

        // Default destination deliberately differs
        // from the source format.
        switch (format) {
        case ImageFormat::PSD:
            rule.destinationFormat =
                ImageFormat::TIFF;
            break;

        case ImageFormat::TIFF:
            rule.destinationFormat =
                ImageFormat::PSD;
            break;

        default:
            rule.destinationFormat =
                ImageFormat::TIFF;
            break;
        }

        m_rules.append(rule);
    }
}

QString ConversionRulesModel::formatText(
    ImageFormat format
    ) const
{
    return formatDisplayName(format);
}

QString ConversionRulesModel::formatDisplayName(
    ImageFormat format
)
{
    switch (format) {
    case ImageFormat::PSD:
        return QStringLiteral("PSD");

    case ImageFormat::TIFF:
        return QStringLiteral("TIFF");

    case ImageFormat::JPEG:
        return QStringLiteral("JPEG");

    case ImageFormat::PNG:
        return QStringLiteral("PNG");

    case ImageFormat::WebP:
        return QStringLiteral("WebP");

    case ImageFormat::BMP:
        return QStringLiteral("BMP");
    }

    return {};
}

QList<ImageFormat>
ConversionRulesModel::availableFormats()
{
    return {
        ImageFormat::PSD,
        ImageFormat::TIFF,
        ImageFormat::JPEG,
        ImageFormat::PNG,
        ImageFormat::WebP,
        ImageFormat::BMP
    };
}

ConversionSettings
ConversionRulesModel::settings() const
{
    ConversionSettings result;

    result.enabled =
        m_conversionEnabled;

    result.rules =
        m_rules;

    return result;
}

void ConversionRulesModel::setSettings(
    const ConversionSettings &settings
)
{
    beginResetModel();

    m_conversionEnabled =
        settings.enabled;

    if (settings.rules.isEmpty()) {
        createDefaultRules();
    }
    else {
        m_rules = settings.rules;
    }

    endResetModel();
}

void ConversionRulesModel::setConversionEnabled(
    bool enabled
)
{
    if (m_conversionEnabled == enabled) {
        return;
    }

    m_conversionEnabled = enabled;

    if (!m_rules.isEmpty()) {
        emit dataChanged(
            index(0, 0),
            index(
                m_rules.size() - 1,
                ColumnCount - 1
            )
        );
    }

    emit settingsChanged();
}

ConversionRule ConversionRulesModel::ruleAt(
    int row
) const
{
    if (
        row < 0 ||
        row >= m_rules.size()
    ) {
        return {};
    }

    return m_rules.at(row);
}
