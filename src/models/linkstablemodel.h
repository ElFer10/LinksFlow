#pragma once

#include <QAbstractTableModel>
#include <QList>

#include "../domain/linkinfo.h"

class LinksTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column
    {
        ProcessColumn = 0,
        PageColumn,
        NameColumn,
        TypeColumn,
        ColorColumn,
        SizeColumn,
        EffectiveResolutionColumn,
        IccProfileColumn,
        ColumnCount
    };

    explicit LinksTableModel(QObject *parent = nullptr);

    int rowCount(
        const QModelIndex &parent = QModelIndex()
    ) const override;

    int columnCount(
        const QModelIndex &parent = QModelIndex()
    ) const override;

    QVariant data(
        const QModelIndex &index,
        int role = Qt::DisplayRole
    ) const override;

    bool setData(
        const QModelIndex &index,
        const QVariant &value,
        int role = Qt::EditRole
    ) override;

    QVariant headerData(
        int section,
        Qt::Orientation orientation,
        int role = Qt::DisplayRole
    ) const override;

    Qt::ItemFlags flags(
        const QModelIndex &index
    ) const override;

    void setLinks(
        const QList<LinkInfo> &links
    );

    QList<LinkInfo> links() const;

    LinkInfo linkAt(int row) const;
    int processableCount() const;
    int selectedForProcessingCount() const;

signals:
    void processSelectionChanged();

private:
    QString formatFileSize(qint64 bytes) const;

    QString formatResolution(
        const Resolution2D &resolution
    ) const;

    bool resolutionsAreEqual(
        const Resolution2D &resolution
    ) const;

private:
    QList<LinkInfo> m_links;
};
