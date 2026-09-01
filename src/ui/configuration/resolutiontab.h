#pragma once

#include <QWidget>

#include "../../domain/resolutionsettings.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QRadioButton;
class QWidget;

class ResolutionTab : public QWidget
{
    Q_OBJECT

public:
    explicit ResolutionTab(QWidget *parent = nullptr);

    ResolutionSettings settings() const;
    void setSettings(const ResolutionSettings &settings);

signals:
    void settingsChanged();

private:
    void setupUi();
    void setupConnections();

    void updateResolutionUnits();
    void updateOptimizationControls();

    void convertResolutionValues(
        ResolutionUnit oldUnit,
        ResolutionUnit newUnit
        );

private:
    QComboBox *m_resolutionUnitCombo = nullptr;

    QDoubleSpinBox *m_colorResolutionSpin = nullptr;
    QDoubleSpinBox *m_monochromeResolutionSpin = nullptr;

    QRadioButton *m_scaleAndResampleRadio = nullptr;
    QRadioButton *m_resampleOnlyRadio = nullptr;

    QCheckBox *m_cropCheckBox = nullptr;

    QWidget *m_safetyAreaContainer = nullptr;
    QDoubleSpinBox *m_safetyAreaSpin = nullptr;
    QComboBox *m_safetyAreaUnitCombo = nullptr;

    ResolutionUnit m_currentResolutionUnit =
        ResolutionUnit::Ppi;

    bool m_updatingUi = false;
};