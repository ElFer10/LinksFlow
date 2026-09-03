#pragma once

#include <QStyledItemDelegate>

class ConversionFormatDelegate
    : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit ConversionFormatDelegate(
        QObject *parent = nullptr
    );

    QWidget *createEditor(
        QWidget *parent,
        const QStyleOptionViewItem &option,
        const QModelIndex &index
    ) const override;

    void setEditorData(
        QWidget *editor,
        const QModelIndex &index
    ) const override;

    void setModelData(
        QWidget *editor,
        QAbstractItemModel *model,
        const QModelIndex &index
    ) const override;
};
