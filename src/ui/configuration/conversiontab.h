#pragma once

#include <QWidget>

#include "../../domain/conversionsettings.h"

class QCheckBox;
class QTableView;
class ConversionRulesModel;

class ConversionTab : public QWidget
{
    Q_OBJECT

public:
    explicit ConversionTab(
        QWidget *parent = nullptr
    );

    ConversionSettings settings() const;

    void setSettings(
        const ConversionSettings &settings
    );

signals:
    void settingsChanged();

    void optionsRequested(
        int ruleIndex
    );

private:
    void setupUi();
    void setupConnections();

private:
    QCheckBox *m_conversionCheckBox = nullptr;
    QTableView *m_table = nullptr;

    ConversionRulesModel *m_model = nullptr;

    bool m_updatingUi = false;
};
