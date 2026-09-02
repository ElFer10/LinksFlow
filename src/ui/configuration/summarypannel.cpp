#include "summarypannel.h"

#include <QFont>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QStringList>

SummaryPanel::SummaryPanel(QWidget *parent)
    : QFrame(parent)
{
    setupUi();
}

void SummaryPanel::setupUi()
{
    setFrameShape(QFrame::StyledPanel);

    setMinimumWidth(250);
    setMaximumWidth(300);

    auto *layout = new QVBoxLayout(this);

    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(8);

    // ---------------------------------------------------------
    // Title
    // ---------------------------------------------------------

    auto *title =
        new QLabel(tr("Resumen"), this);

    QFont titleFont = title->font();
    titleFont.setBold(true);

    title->setFont(titleFont);

    layout->addWidget(title);

    // ---------------------------------------------------------
    // Preset
    // ---------------------------------------------------------

    auto *presetLabel =
        new QLabel(tr("Preset"), this);

    QFont sectionFont = presetLabel->font();
    sectionFont.setBold(true);

    presetLabel->setFont(sectionFont);

    layout->addSpacing(6);
    layout->addWidget(presetLabel);

    m_presetValue =
        new QLabel(this);

    layout->addWidget(m_presetValue);

    // ---------------------------------------------------------
    // Resolution
    // ---------------------------------------------------------

    auto *resolutionLabel =
        new QLabel(
            tr("Resolución efectiva deseada"),
            this
        );

    resolutionLabel->setFont(sectionFont);

    layout->addSpacing(8);
    layout->addWidget(resolutionLabel);

    m_colorResolutionValue =
        new QLabel(this);

    m_monochromeResolutionValue =
        new QLabel(this);

    layout->addWidget(m_colorResolutionValue);
    layout->addWidget(m_monochromeResolutionValue);

    // ---------------------------------------------------------
    // Optimization
    // ---------------------------------------------------------

    auto *optimizationLabel =
        new QLabel(
            tr("Método de optimización"),
            this
        );

    optimizationLabel->setFont(sectionFont);

    layout->addSpacing(8);
    layout->addWidget(optimizationLabel);

    m_optimizationMethodValue =
        new QLabel(this);

    m_optimizationMethodValue->setWordWrap(true);

    m_cropValue =
        new QLabel(this);

    m_cropValue->setWordWrap(true);

    m_safetyAreaValue =
        new QLabel(this);

    layout->addWidget(
        m_optimizationMethodValue
    );

    layout->addWidget(
        m_cropValue
    );

    layout->addWidget(
        m_safetyAreaValue
    );

    // ---------------------------------------------------------
    // Editing
    // ---------------------------------------------------------

    auto *editingLabel =
        new QLabel(
            tr("Edición de imágenes"),
            this
        );

    editingLabel->setFont(sectionFont);

    layout->addSpacing(8);
    layout->addWidget(editingLabel);

    m_editingValue =
        new QLabel(tr("Sin configurar"), this);
m_editingValue->setWordWrap(true);

    layout->addWidget(m_editingValue);

    // ---------------------------------------------------------
    // Conversion
    // ---------------------------------------------------------

    auto *conversionLabel =
        new QLabel(
            tr("Conversión de imágenes"),
            this
        );

    conversionLabel->setFont(sectionFont);

    layout->addSpacing(8);
    layout->addWidget(conversionLabel);

    m_conversionValue =
        new QLabel(tr("Desactivada"), this);

    layout->addWidget(m_conversionValue);

    layout->addStretch();

    // ---------------------------------------------------------
    // Reset
    // ---------------------------------------------------------

    m_resetButton =
        new QPushButton(
            tr("Resetear la configuración"),
            this
        );

    layout->addWidget(m_resetButton);

    connect(
        m_resetButton,
        &QPushButton::clicked,
        this,
        &SummaryPanel::resetRequested
    );
}

void SummaryPanel::setPreset(
    const Preset &preset
)
{
    const ResolutionSettings &resolution =
        preset.resolution;

    const QString unit =
        resolutionUnitText(resolution.unit);

    m_presetValue->setText(
        preset.name
    );

    m_colorResolutionValue->setText(
        tr("Color / Escala de grises: %1 %2")
            .arg(resolution.colorResolution, 0, 'f', 0)
            .arg(unit)
    );

    m_monochromeResolutionValue->setText(
        tr("Monochrome: %1 %2")
            .arg(
                resolution.monochromeResolution,
                0,
                'f',
                0
            )
            .arg(unit)
    );

    if (
        resolution.optimizationMethod ==
        OptimizationMethod::ScaleAndResample
    ) {
        m_optimizationMethodValue->setText(
            tr("Escalar y cambiar resolución")
        );

        if (resolution.cropToInDesignFrame) {
            m_cropValue->setText(
                tr("Recortar al tamaño de InDesign")
            );

            QString safetyUnit;

            switch (resolution.safetyAreaUnit) {
            case ResolutionSettings::SafetyAreaUnit::Centimeters:
                safetyUnit = tr("cm");
                break;

            case ResolutionSettings::SafetyAreaUnit::Inches:
                safetyUnit = tr("in");
                break;

            default:
                safetyUnit = tr("mm");
                break;
            }

            m_safetyAreaValue->setText(
                tr("Área de seguridad: %1 %2")
                    .arg(
                        resolution.safetyArea,
                        0,
                        'f',
                        2
                    )
                    .arg(safetyUnit)
            );
        }
        else {
            m_cropValue->setText(
                tr("Sin recorte")
            );

            m_safetyAreaValue->clear();
        }
    }
    else {
        m_optimizationMethodValue->setText(
            tr("Solo cambiar resolución")
        );

        m_cropValue->setText(
            tr("Sin escalado ni recorte")
        );

        m_safetyAreaValue->clear();
    }
m_editingValue->setText(
    imageEditingSummary(
        preset.imageEditing
    )
);
}

QString SummaryPanel::resolutionUnitText(
    ResolutionUnit unit
) const
{
    if (unit == ResolutionUnit::Ppi) {
        return tr("ppi");
    }

    return tr("px/cm");
}

QString SummaryPanel::colorModeText(
    ColorMode mode
) const
{
    switch (mode) {
    case ColorMode::CMYK:
        return tr("CMYK");

    case ColorMode::Grayscale:
        return tr("Escala de grises");

    default:
        return tr("RGB");
    }
}

QString SummaryPanel::imageEditingSummary(
    const ImageEditingSettings &settings
) const
{
    QStringList lines;

    if (settings.changeColorMode) {
        lines << tr("Modo: %1 → %2")
                     .arg(
                         colorModeText(
                             settings.sourceColorMode
                         ),
                         colorModeText(
                             settings.destinationColorMode
                         )
                     );
    }

    if (settings.changeColorProfile) {
        lines << tr("Perfil ICC: %1")
                     .arg(settings.iccProfile);
    }

    if (settings.removeHiddenLayers) {
        lines << tr("Eliminar capas ocultas");
    }

    if (settings.mergeVisibleLayers) {
        lines << tr("Combinar capas visibles");
    }

    if (settings.flattenImage) {
        lines << tr("Acoplar imagen");
    }

    if (
        settings.alphaChannels ==
        AlphaChannelHandling::Remove
    ) {
        lines << tr("Eliminar canales alfa");
    }
    else {
        lines << tr("Conservar canales alfa");
    }

    if (lines.isEmpty()) {
        return tr("Sin cambios");
    }

    return lines.join(
        QStringLiteral("\n")
    );
}
