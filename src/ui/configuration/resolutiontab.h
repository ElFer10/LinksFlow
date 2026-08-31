#pragma once

#include <QWidget>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QRadioButton;
class QWidget;

class ResolutionTab : public QWidget
{
  Q_OBJECT

  public:
    explicit ResolutionTab(QWidget *parent=nullptr);

  signals:
    void settingsChanged();

  private:
    void setupUi();
    void setupConnections();

    void updateResolutionUnits();
    void updateOptimizationControls();

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
};