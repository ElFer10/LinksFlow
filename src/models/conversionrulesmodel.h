#pragma once

#include <QAbstractTableModel>

#include "../domain/conversionsettings.h"

class ConversionRulesModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum Column
    {
        EnabledColumn = 0,
        SourceFormatColumn,
        DestinationFormatColumn,
        OptionsColumn,
        ColumnCount
    };

    explicit ConversionRulesModel( QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;

    ConversionSettings settings() const;

    void setSettings(const ConversionSettings &settings);

    void setConversionEnabled(bool enabled);

    ConversionRule ruleAt(int row) const;

    static QString formatDisplayName(ImageFormat format
    );

    static QList<ImageFormat> availableFormats();

signals:
    void settingsChanged();
    void optionsRequested(int row);

private:
    void createDefaultRules();

    QString formatText(ImageFormat format) const;

private:
    QList<ConversionRule> m_rules;

    bool m_conversionEnabled = false;
};
