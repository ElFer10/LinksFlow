#include "resolutiontab.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

ResolutionTab::ResolutionTab(QWidget *parent) : QWidget(parent) {
  setupUi();
  setupConnections();

  updateResolutionUnits();
  updateOptimizationControls();
}

void ResolutionTab::setupUi() {
  auto *mainLayout = new QVBoxLayout(this);

  mainLayout->setContentsMargins(16, 16, 16, 16);
  mainLayout->setSpacing(18);

  // =========================================================
  // Resolución efectiva
  // =========================================================

  auto *resolutionGroup =
      new QGroupBox(tr("Resolución efectiva deseada"), this);

  auto *resolutionLayout = new QVBoxLayout(resolutionGroup);

  resolutionLayout->setSpacing(12);

  // nidades

  auto *unitLayout = new QHBoxLayout;

  auto *unitLabel = new QLabel(tr("Unidades:"), resolutionGroup);

  m_resolutionUnitCombo = new QComboBox(resolutionGroup);

  m_resolutionUnitCombo->addItem(tr("pixels/inch"), QStringLiteral("ppi"));
  m_resolutionUnitCombo->addItem(tr("pixels/cm"), QStringLiteral("ppcm"));

  unitLayout->addWidget(unitLabel);
  unitLayout->addWidget(m_resolutionUnitCombo);
  unitLayout->addStretch();

  resolutionLayout->addLayout(unitLayout);

  // ---------------------------------------------------------
  // Resoluciones
  // ---------------------------------------------------------

  auto *resolutionValuesLayout = new QVBoxLayout;
  resolutionValuesLayout->setSpacing(20);

  // Color / grayscale
  auto *colorGroup =
      new QGroupBox(tr("Color / Escala de grises"), resolutionGroup);
  auto *colorLayout = new QHBoxLayout(colorGroup);

  m_colorResolutionSpin = new QDoubleSpinBox(colorGroup);

  m_colorResolutionSpin->setRange(1.0, 10000.0);
  m_colorResolutionSpin->setDecimals(0);
  m_colorResolutionSpin->setSingleStep(10.0);
  m_colorResolutionSpin->setValue(300.0);

  colorLayout->addWidget(m_colorResolutionSpin);
  colorLayout->addStretch();

  // Monochrome
  auto *monochromeGroup =
      new QGroupBox(tr("Escala monocromática (Blanco y negro)"), resolutionGroup);
  auto *monochromeLayout = new QHBoxLayout(monochromeGroup);
  m_monochromeResolutionSpin = new QDoubleSpinBox(monochromeGroup);

  m_monochromeResolutionSpin->setRange(1.0, 10000.0);
  m_monochromeResolutionSpin->setDecimals(0);
  m_monochromeResolutionSpin->setSingleStep(50.0);
  m_monochromeResolutionSpin->setValue(1200.0);

  monochromeLayout->addWidget(m_monochromeResolutionSpin);

  monochromeLayout->addStretch();

  resolutionValuesLayout->addWidget(colorGroup);
  resolutionValuesLayout->addWidget(monochromeGroup);

  resolutionLayout->addLayout(resolutionValuesLayout);

  mainLayout->addWidget(resolutionGroup);

  // =========================================================
  // Método de Optimizacion
  // =========================================================

  auto *optimizationGroup = new QGroupBox(tr("Método de optimización"), this);

  auto *optimizationLayout = new QVBoxLayout(optimizationGroup);

  optimizationLayout->setSpacing(12);

  // ---------------------------------------------------------
  // Scale + resample
  // ---------------------------------------------------------

  m_scaleAndResampleRadio = new QRadioButton(
      tr("Escalar imágenes y cambiar resolución a la resolución deseada"),
      optimizationGroup);

  m_scaleAndResampleRadio->setChecked(true);

  optimizationLayout->addWidget(m_scaleAndResampleRadio);

  auto *dependentControls = new QWidget(optimizationGroup);

  auto *dependentLayout = new QVBoxLayout(dependentControls);

  dependentLayout->setContentsMargins(24, 0, 0, 0);

  dependentLayout->setSpacing(8);

  m_cropCheckBox = new QCheckBox(tr("Recortar imágenes al tamaño de InDesign"),
                                 dependentControls);

  m_cropCheckBox->setChecked(true);

  dependentLayout->addWidget(m_cropCheckBox);

  // Área de seguridad
  m_safetyAreaContainer = new QWidget(dependentControls);

  auto *safetyLayout = new QHBoxLayout(m_safetyAreaContainer);

  safetyLayout->setContentsMargins(0, 0, 0, 0);

  auto *safetyLabel =
      new QLabel(tr("Área de seguridad:"), m_safetyAreaContainer);

  m_safetyAreaSpin = new QDoubleSpinBox(m_safetyAreaContainer);

  m_safetyAreaSpin->setRange(0.0, 1000.0);
  m_safetyAreaSpin->setDecimals(2);
  m_safetyAreaSpin->setSingleStep(0.5);
  m_safetyAreaSpin->setValue(3.0);

  m_safetyAreaUnitCombo = new QComboBox(m_safetyAreaContainer);

  m_safetyAreaUnitCombo->addItems({tr("mm"), tr("cm"), tr("in")});

  safetyLayout->addWidget(safetyLabel);
  safetyLayout->addWidget(m_safetyAreaSpin);
  safetyLayout->addWidget(m_safetyAreaUnitCombo);
  safetyLayout->addStretch();

  dependentLayout->addWidget(m_safetyAreaContainer);

  optimizationLayout->addWidget(dependentControls);

  // ---------------------------------------------------------
  // Resample only
  // ---------------------------------------------------------

  m_resampleOnlyRadio = new QRadioButton(tr("Solo cambiar la resolución "
                                            "a la resolución deseada"),
                                         optimizationGroup);

  optimizationLayout->addWidget(m_resampleOnlyRadio);

  auto *explanationLabel =
      new QLabel(tr("Las imágenes no serán escaladas ni recortadas. "
                    "Las dimensiones de las imágenes no variarán."),
                 optimizationGroup);

  explanationLabel->setWordWrap(true);

  // Secondary/disabled-looking native palette text.
  QPalette secondaryPalette = explanationLabel->palette();

  secondaryPalette.setColor(
      QPalette::WindowText,
      secondaryPalette.color(QPalette::Disabled, QPalette::WindowText));

  explanationLabel->setPalette(secondaryPalette);

  auto *explanationLayout = new QHBoxLayout;

  explanationLayout->setContentsMargins(24, 0, 0, 0);

  explanationLayout->addWidget(explanationLabel);

  optimizationLayout->addLayout(explanationLayout);

  mainLayout->addWidget(optimizationGroup);

  mainLayout->addStretch();
}

void ResolutionTab::setupConnections() {
  connect(m_resolutionUnitCombo, &QComboBox::currentIndexChanged, this,
          [this]() {
            updateResolutionUnits();
            emit settingsChanged();
          });

  connect(m_colorResolutionSpin, &QDoubleSpinBox::valueChanged, this,
          &ResolutionTab::settingsChanged);

  connect(m_monochromeResolutionSpin, &QDoubleSpinBox::valueChanged, this,
          &ResolutionTab::settingsChanged);

  connect(m_scaleAndResampleRadio, &QRadioButton::toggled, this, [this]() {
    updateOptimizationControls();
    emit settingsChanged();
  });

  connect(m_resampleOnlyRadio, &QRadioButton::toggled, this,
          &ResolutionTab::settingsChanged);

  connect(m_cropCheckBox, &QCheckBox::toggled, this, [this]() {
    updateOptimizationControls();
    emit settingsChanged();
  });

  connect(m_safetyAreaSpin, &QDoubleSpinBox::valueChanged, this,
          &ResolutionTab::settingsChanged);

  connect(m_safetyAreaUnitCombo, &QComboBox::currentIndexChanged, this,
          &ResolutionTab::settingsChanged);
}

void ResolutionTab::updateResolutionUnits() {
  const QString unit = m_resolutionUnitCombo->currentData().toString();

  if (unit == QStringLiteral("ppi")) {
    m_colorResolutionSpin->setSuffix(tr(" ppi"));

    m_monochromeResolutionSpin->setSuffix(tr(" ppi"));
  } else {
    m_colorResolutionSpin->setSuffix(tr(" px/cm"));

    m_monochromeResolutionSpin->setSuffix(tr(" px/cm"));
  }
}

void ResolutionTab::updateOptimizationControls() {
  const bool scaleAndResample = m_scaleAndResampleRadio->isChecked();

  m_cropCheckBox->setEnabled(scaleAndResample);

  const bool safetyAreaEnabled =
      scaleAndResample && m_cropCheckBox->isChecked();

  m_safetyAreaContainer->setEnabled(safetyAreaEnabled);
}
