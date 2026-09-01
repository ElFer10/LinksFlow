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
#include <QSignalBlocker>

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

    auto *resolutionGroup = new QGroupBox(tr("Resolución efectiva deseada"), this);

    auto *resolutionLayout = new QVBoxLayout(resolutionGroup);

    resolutionLayout->setSpacing(12);

    // Unidades

    auto *unitLayout = new QHBoxLayout;

    auto *unitLabel = new QLabel(tr("Unidades:"), resolutionGroup);

    m_resolutionUnitCombo = new QComboBox(resolutionGroup);

    m_resolutionUnitCombo->addItem( tr("pixels/inch"), QVariant::fromValue(ResolutionUnit::Ppi));
    m_resolutionUnitCombo->addItem( tr("pixels/cm"), QVariant::fromValue(ResolutionUnit::PixelsPerCm));

    unitLayout->addWidget(unitLabel);
    unitLayout->addWidget(m_resolutionUnitCombo);
    unitLayout->addStretch();

    resolutionLayout->addLayout(unitLayout);

    // ---------------------------------------------------------
    // Resoluciones
    // ---------------------------------------------------------


    auto *resolutionValuesLayout = new QHBoxLayout;
    resolutionValuesLayout->setSpacing(20);

    // Color / grayscale
    auto *colorGroup = new QGroupBox(tr("Color / Escala de grises"), resolutionGroup);
    auto *colorLayout = new QHBoxLayout(colorGroup);

    auto *colorResolutionLabel =
        new QLabel(tr("Resolución deseada:"), colorGroup);

    m_colorResolutionSpin = new QDoubleSpinBox(colorGroup);

    colorResolutionLabel->setBuddy(m_colorResolutionSpin);

    m_colorResolutionSpin->setRange(1.0, 10000.0);
    m_colorResolutionSpin->setDecimals(0);
    m_colorResolutionSpin->setSingleStep(10.0);
    m_colorResolutionSpin->setValue(300.0);

    colorLayout->addWidget(colorResolutionLabel);
    colorLayout->addWidget(m_colorResolutionSpin);
    colorLayout->addStretch();

    // Monochrome
    auto *monochromeGroup =
        new QGroupBox(tr("Escala monocromática (Blanco y negro)"), resolutionGroup);
    auto *monochromeLayout = new QHBoxLayout(monochromeGroup);

    auto *monochromeResolutionLabel =
        new QLabel(tr("Resolución deseada:"), monochromeGroup);

    m_monochromeResolutionSpin = new QDoubleSpinBox(monochromeGroup);

    monochromeResolutionLabel->setBuddy(m_monochromeResolutionSpin);

    m_monochromeResolutionSpin->setRange(1.0, 10000.0);
    m_monochromeResolutionSpin->setDecimals(0);
    m_monochromeResolutionSpin->setSingleStep(50.0);
    m_monochromeResolutionSpin->setValue(1200.0);

    monochromeLayout->addWidget(monochromeResolutionLabel);
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

void ResolutionTab::setupConnections()
{
    connect(
        m_resolutionUnitCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this]()
        {
            if (m_updatingUi) {
                return;
            }

            const ResolutionUnit newUnit =
                m_resolutionUnitCombo->currentData()
                    .value<ResolutionUnit>();

            if (newUnit != m_currentResolutionUnit) {
                convertResolutionValues(
                    m_currentResolutionUnit,
                    newUnit
                    );

                m_currentResolutionUnit = newUnit;
            }

            updateResolutionUnits();

            emit settingsChanged();
        }
        );

    connect(
        m_colorResolutionSpin,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]()
        {
            if (!m_updatingUi) {
                emit settingsChanged();
            }
        }
        );

    connect(
        m_monochromeResolutionSpin,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]()
        {
            if (!m_updatingUi) {
                emit settingsChanged();
            }
        }
        );

    connect(
        m_scaleAndResampleRadio,
        &QRadioButton::toggled,
        this,
        [this]()
        {
            updateOptimizationControls();

            if (!m_updatingUi) {
                emit settingsChanged();
            }
        }
        );

    connect(
        m_resampleOnlyRadio,
        &QRadioButton::toggled,
        this,
        [this]()
        {
            if (!m_updatingUi) {
                emit settingsChanged();
            }
        }
        );

    connect(
        m_cropCheckBox,
        &QCheckBox::toggled,
        this,
        [this]()
        {
            updateOptimizationControls();

            if (!m_updatingUi) {
                emit settingsChanged();
            }
        }
        );

    connect(
        m_safetyAreaSpin,
        &QDoubleSpinBox::valueChanged,
        this,
        [this]()
        {
            if (!m_updatingUi) {
                emit settingsChanged();
            }
        }
        );

    connect(
        m_safetyAreaUnitCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this]()
        {
            if (!m_updatingUi) {
                emit settingsChanged();
            }
        }
        );
}

void ResolutionTab::updateResolutionUnits()
{
    const ResolutionUnit unit =
        m_resolutionUnitCombo
            ->currentData()
            .value<ResolutionUnit>();

    if (unit == ResolutionUnit::Ppi) {
        m_colorResolutionSpin->setSuffix(
            tr(" ppi")
            );

        m_monochromeResolutionSpin->setSuffix(
            tr(" ppi")
            );
    }
    else {
        m_colorResolutionSpin->setSuffix(
            tr(" px/cm")
            );

        m_monochromeResolutionSpin->setSuffix(
            tr(" px/cm")
            );
    }
}

void ResolutionTab::updateOptimizationControls() {
    const bool scaleAndResample = m_scaleAndResampleRadio->isChecked();

    m_cropCheckBox->setEnabled(scaleAndResample);

    const bool safetyAreaEnabled =
        scaleAndResample && m_cropCheckBox->isChecked();

    m_safetyAreaContainer->setEnabled(safetyAreaEnabled);
}


void ResolutionTab::convertResolutionValues(
    ResolutionUnit oldUnit,
    ResolutionUnit newUnit
    )
{
    if (oldUnit == newUnit) {
        return;
    }

    constexpr double centimetersPerInch = 2.54;

    double factor = 1.0;

    if (
        oldUnit == ResolutionUnit::Ppi &&
        newUnit == ResolutionUnit::PixelsPerCm
        ) {
        factor = 1.0 / centimetersPerInch;
    }
    else if (
        oldUnit == ResolutionUnit::PixelsPerCm &&
        newUnit == ResolutionUnit::Ppi
        ) {
        factor = centimetersPerInch;
    }

    const QSignalBlocker colorBlocker(
        m_colorResolutionSpin
        );

    const QSignalBlocker monochromeBlocker(
        m_monochromeResolutionSpin
        );

    m_colorResolutionSpin->setValue(
        m_colorResolutionSpin->value() * factor
        );

    m_monochromeResolutionSpin->setValue(
        m_monochromeResolutionSpin->value() * factor

        );

}

ResolutionSettings ResolutionTab::settings() const
{
    ResolutionSettings result;

    result.unit =
        m_resolutionUnitCombo
            ->currentData()
            .value<ResolutionUnit>();

    result.colorResolution =
        m_colorResolutionSpin->value();

    result.monochromeResolution =
        m_monochromeResolutionSpin->value();

    result.optimizationMethod =
        m_scaleAndResampleRadio->isChecked()
            ? OptimizationMethod::ScaleAndResample
            : OptimizationMethod::ResampleOnly;

    result.cropToInDesignFrame =
        m_cropCheckBox->isChecked();

    result.safetyArea =
        m_safetyAreaSpin->value();

    switch (m_safetyAreaUnitCombo->currentIndex()) {
    case 1:
        result.safetyAreaUnit =
            ResolutionSettings::SafetyAreaUnit::Centimeters;
        break;

    case 2:
        result.safetyAreaUnit =
            ResolutionSettings::SafetyAreaUnit::Inches;
        break;

    default:
        result.safetyAreaUnit =
            ResolutionSettings::SafetyAreaUnit::Millimeters;
        break;
    }

    return result;
}

void ResolutionTab::setSettings(
    const ResolutionSettings &settings
    )
{
    m_updatingUi = true;

    const int unitIndex =
        m_resolutionUnitCombo->findData(
            QVariant::fromValue(settings.unit)
            );

    if (unitIndex >= 0) {
        m_resolutionUnitCombo->setCurrentIndex(
            unitIndex
            );
    }

    m_currentResolutionUnit =
        settings.unit;

    m_colorResolutionSpin->setValue(
        settings.colorResolution
        );

    m_monochromeResolutionSpin->setValue(
        settings.monochromeResolution
        );

    m_scaleAndResampleRadio->setChecked(
        settings.optimizationMethod ==
        OptimizationMethod::ScaleAndResample
        );

    m_resampleOnlyRadio->setChecked(
        settings.optimizationMethod ==
        OptimizationMethod::ResampleOnly
        );

    m_cropCheckBox->setChecked(
        settings.cropToInDesignFrame
        );

    m_safetyAreaSpin->setValue(
        settings.safetyArea
        );

    switch (settings.safetyAreaUnit) {
    case ResolutionSettings::SafetyAreaUnit::Centimeters:
        m_safetyAreaUnitCombo->setCurrentIndex(1);
        break;

    case ResolutionSettings::SafetyAreaUnit::Inches:
        m_safetyAreaUnitCombo->setCurrentIndex(2);
        break;

    default:
        m_safetyAreaUnitCombo->setCurrentIndex(0);
        break;
    }

    updateResolutionUnits();
    updateOptimizationControls();

    m_updatingUi = false;
}
