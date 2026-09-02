#include "imageeditingtab.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QVBoxLayout>

ImageEditingTab::ImageEditingTab(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    setupConnections();

    updateColorModeControls();
    updateColorProfileControls();
}

void ImageEditingTab::setupUi()
{
    auto *mainLayout = new QHBoxLayout(this);

    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(16);

    // =========================================================
    // Color modes
    // =========================================================

    auto *colorGroup =
        new QGroupBox(tr("Modos de color"), this);

    auto *colorLayout =
        new QVBoxLayout(colorGroup);

    colorLayout->setSpacing(12);

    m_changeColorModeCheckBox =
        new QCheckBox(
            tr("Cambiar modos de color"),
            colorGroup
        );

    colorLayout->addWidget(
        m_changeColorModeCheckBox
    );

    m_colorModeControlsContainer =
        new QWidget(colorGroup);

    auto *colorModeLayout =
        new QFormLayout(m_colorModeControlsContainer);

    colorModeLayout->setContentsMargins(
        20,
        0,
        0,
        0
    );

    m_sourceColorModeCombo =
        new QComboBox(m_colorModeControlsContainer);

    m_destinationColorModeCombo =
        new QComboBox(m_colorModeControlsContainer);

    const QList<QPair<QString, ColorMode>> modes = {
        {tr("RGB"), ColorMode::RGB},
        {tr("CMYK"), ColorMode::CMYK},
        {tr("Escala de grises"), ColorMode::Grayscale}
    };

    for (const auto &mode : modes) {
        m_sourceColorModeCombo->addItem(
            mode.first,
            static_cast<int>(mode.second)
        );

        m_destinationColorModeCombo->addItem(
            mode.first,
            static_cast<int>(mode.second)
        );
    }

    colorModeLayout->addRow(
        tr("Origen:"),
        m_sourceColorModeCombo
    );

    colorModeLayout->addRow(
        tr("Destino:"),
        m_destinationColorModeCombo
    );

    colorLayout->addWidget(
        m_colorModeControlsContainer
    );

    // ---------------------------------------------------------
    // ICC profile
    // ---------------------------------------------------------

    m_changeColorProfileCheckBox =
        new QCheckBox(
            tr("Cambiar perfil"),
            colorGroup
        );

    colorLayout->addWidget(
        m_changeColorProfileCheckBox
    );

    m_colorProfileControlsContainer =
        new QWidget(colorGroup);

    auto *profileLayout =
        new QFormLayout(
            m_colorProfileControlsContainer
        );

    profileLayout->setContentsMargins(
        20,
        0,
        0,
        0
    );

    m_iccProfileCombo =
        new QComboBox(
            m_colorProfileControlsContainer
        );

    m_iccProfileCombo->setEditable(false);

    // Lista temporal.
    // Más adelante la cargaremos desde perfiles ICC reales.
    m_iccProfileCombo->addItems(
        {
            tr("sRGB IEC61966-2.1"),
            tr("Adobe RGB (1998)"),
            tr("Display P3"),
            tr("Coated FOGRA39"),
            tr("PSO Coated v3")
        }
    );

    profileLayout->addRow(
        tr("Perfil ICC:"),
        m_iccProfileCombo
    );

    colorLayout->addWidget(
        m_colorProfileControlsContainer
    );

    colorLayout->addStretch();

    // =========================================================
    // Layers and channels
    // =========================================================

    auto *layersGroup =
        new QGroupBox(
            tr("Capas y canales"),
            this
        );

    auto *layersLayout =
        new QVBoxLayout(layersGroup);

    layersLayout->setSpacing(10);

    auto *layersLabel =
        new QLabel(
            tr("Capas"),
            layersGroup
        );

    QFont subsectionFont =
        layersLabel->font();

    subsectionFont.setBold(true);

    layersLabel->setFont(
        subsectionFont
    );

    layersLayout->addWidget(
        layersLabel
    );

    m_removeHiddenLayersCheckBox =
        new QCheckBox(
            tr("Eliminar capas ocultas"),
            layersGroup
        );

    m_mergeVisibleLayersCheckBox =
        new QCheckBox(
            tr("Combinar capas visibles"),
            layersGroup
        );

    m_flattenImageCheckBox =
        new QCheckBox(
            tr("Acoplar imagen"),
            layersGroup
        );

    layersLayout->addWidget(
        m_removeHiddenLayersCheckBox
    );

    layersLayout->addWidget(
        m_mergeVisibleLayersCheckBox
    );

    layersLayout->addWidget(
        m_flattenImageCheckBox
    );

    layersLayout->addSpacing(12);

    auto *channelsLabel =
        new QLabel(
            tr("Canales alfa"),
            layersGroup
        );

    channelsLabel->setFont(
        subsectionFont
    );

    layersLayout->addWidget(
        channelsLabel
    );

    m_keepAlphaChannelsRadio =
        new QRadioButton(
            tr("Conservar canales alfa"),
            layersGroup
        );

    m_removeAlphaChannelsRadio =
        new QRadioButton(
            tr("Eliminar canales alfa"),
            layersGroup
        );

    m_keepAlphaChannelsRadio->setChecked(
        true
    );

    layersLayout->addWidget(
        m_keepAlphaChannelsRadio
    );

    layersLayout->addWidget(
        m_removeAlphaChannelsRadio
    );

    layersLayout->addStretch();

    mainLayout->addWidget(colorGroup, 1);
    mainLayout->addWidget(layersGroup, 1);
}

void ImageEditingTab::setupConnections()
{
    connect(
        m_changeColorModeCheckBox,
        &QCheckBox::toggled,
        this,
        [this]()
        {
            updateColorModeControls();

            if (!m_updatingUi) {
                emit settingsChanged();
            }
        }
    );

    connect(
        m_sourceColorModeCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this]()
        {
            if (!m_updatingUi) {
                emit settingsChanged();
            }
        }
    );

    connect(
        m_destinationColorModeCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this]()
        {
            if (!m_updatingUi) {
                emit settingsChanged();
            }
        }
    );

    connect(
        m_changeColorProfileCheckBox,
        &QCheckBox::toggled,
        this,
        [this]()
        {
            updateColorProfileControls();

            if (!m_updatingUi) {
                emit settingsChanged();
            }
        }
    );

    connect(
        m_iccProfileCombo,
        &QComboBox::currentIndexChanged,
        this,
        [this]()
        {
            if (!m_updatingUi) {
                emit settingsChanged();
            }
        }
    );

    connect(
        m_removeHiddenLayersCheckBox,
        &QCheckBox::toggled,
        this,
        [this]()
        {
            if (!m_updatingUi) {
                emit settingsChanged();
            }
        }
    );

    connect(
        m_mergeVisibleLayersCheckBox,
        &QCheckBox::toggled,
        this,
        [this]()
        {
            if (!m_updatingUi) {
                emit settingsChanged();
            }
        }
    );

    connect(
        m_flattenImageCheckBox,
        &QCheckBox::toggled,
        this,
        [this]()
        {
            if (!m_updatingUi) {
                emit settingsChanged();
            }
        }
    );

    connect(
        m_keepAlphaChannelsRadio,
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
        m_removeAlphaChannelsRadio,
        &QRadioButton::toggled,
        this,
        [this]()
        {
            if (!m_updatingUi) {
                emit settingsChanged();
            }
        }
    );
}

void ImageEditingTab::updateColorModeControls()
{
    m_colorModeControlsContainer->setEnabled(
        m_changeColorModeCheckBox->isChecked()
    );
}

void ImageEditingTab::updateColorProfileControls()
{
    m_colorProfileControlsContainer->setEnabled(
        m_changeColorProfileCheckBox->isChecked()
    );
}

ImageEditingSettings ImageEditingTab::settings() const
{
    ImageEditingSettings result;

    result.changeColorMode =
        m_changeColorModeCheckBox->isChecked();

    result.sourceColorMode =
        static_cast<ColorMode>(
            m_sourceColorModeCombo
                ->currentData()
                .toInt()
        );

    result.destinationColorMode =
        static_cast<ColorMode>(
            m_destinationColorModeCombo
                ->currentData()
                .toInt()
        );

    result.changeColorProfile =
        m_changeColorProfileCheckBox->isChecked();

    result.iccProfile =
        m_iccProfileCombo->currentText();

    result.removeHiddenLayers =
        m_removeHiddenLayersCheckBox->isChecked();

    result.mergeVisibleLayers =
        m_mergeVisibleLayersCheckBox->isChecked();

    result.flattenImage =
        m_flattenImageCheckBox->isChecked();

    result.alphaChannels =
        m_removeAlphaChannelsRadio->isChecked()
            ? AlphaChannelHandling::Remove
            : AlphaChannelHandling::Keep;

    return result;
}

void ImageEditingTab::setSettings(
    const ImageEditingSettings &settings
)
{
    m_updatingUi = true;

    m_changeColorModeCheckBox->setChecked(
        settings.changeColorMode
    );

    int index =
        m_sourceColorModeCombo->findData(
            static_cast<int>(
                settings.sourceColorMode
            )
        );

    if (index >= 0) {
        m_sourceColorModeCombo->setCurrentIndex(
            index
        );
    }

    index =
        m_destinationColorModeCombo->findData(
            static_cast<int>(
                settings.destinationColorMode
            )
        );

    if (index >= 0) {
        m_destinationColorModeCombo->setCurrentIndex(
            index
        );
    }

    m_changeColorProfileCheckBox->setChecked(
        settings.changeColorProfile
    );

    if (!settings.iccProfile.isEmpty()) {
        index =
            m_iccProfileCombo->findText(
                settings.iccProfile
            );

        if (index >= 0) {
            m_iccProfileCombo->setCurrentIndex(
                index
            );
        }
    }

    m_removeHiddenLayersCheckBox->setChecked(
        settings.removeHiddenLayers
    );

    m_mergeVisibleLayersCheckBox->setChecked(
        settings.mergeVisibleLayers
    );

    m_flattenImageCheckBox->setChecked(
        settings.flattenImage
    );

    m_keepAlphaChannelsRadio->setChecked(
        settings.alphaChannels ==
        AlphaChannelHandling::Keep
    );

    m_removeAlphaChannelsRadio->setChecked(
        settings.alphaChannels ==
        AlphaChannelHandling::Remove
    );

    updateColorModeControls();
    updateColorProfileControls();

    m_updatingUi = false;
}
