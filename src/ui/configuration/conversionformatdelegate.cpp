#include "conversionformatdelegate.h"

#include "../../models/conversionrulesmodel.h"

#include <QComboBox>

ConversionFormatDelegate::ConversionFormatDelegate(
    QObject *parent
)
    : QStyledItemDelegate(parent)
{
}

QWidget *ConversionFormatDelegate::createEditor(
    QWidget *parent,
    const QStyleOptionViewItem &,
    const QModelIndex &index
) const
{
    auto *combo =
        new QComboBox(parent);

    const auto *model =
        qobject_cast<
            const ConversionRulesModel *
        >(index.model());

    if (!model) {
        return combo;
    }

    const ConversionRule rule =
        model->ruleAt(index.row());

    for (
        ImageFormat format :
        ConversionRulesModel::availableFormats()
    ) {
        // Don't offer conversion to same format.
        if (format == rule.sourceFormat) {
            continue;
        }

        combo->addItem(
            ConversionRulesModel::
                formatDisplayName(format),
            static_cast<int>(format)
        );
    }

    return combo;
}

void ConversionFormatDelegate::setEditorData(
    QWidget *editor,
    const QModelIndex &index
) const
{
    auto *combo =
        qobject_cast<QComboBox *>(editor);

    if (!combo) {
        return;
    }

    const auto *model =
        qobject_cast<
            const ConversionRulesModel *
        >(index.model());

    if (!model) {
        return;
    }

    const ConversionRule rule =
        model->ruleAt(index.row());

    const int comboIndex =
        combo->findData(
            static_cast<int>(
                rule.destinationFormat
            )
        );

    if (comboIndex >= 0) {
        combo->setCurrentIndex(comboIndex);
    }
}

void ConversionFormatDelegate::setModelData(
    QWidget *editor,
    QAbstractItemModel *model,
    const QModelIndex &index
) const
{
    auto *combo =
        qobject_cast<QComboBox *>(editor);

    if (!combo) {
        return;
    }

    model->setData(
        index,
        combo->currentData(),
        Qt::EditRole
    );
}
