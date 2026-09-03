#include "conversionoptionsdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace OptionKeys
{
constexpr auto Quality = "quality";
constexpr auto Compression = "compression";
constexpr auto MaximizeCompatibility =
    "maximizeCompatibility";
constexpr auto Lossless = "lossless";
}

ConversionOptionsDialog::ConversionOptionsDialog(
    ImageFormat format,
    const QVariantMap &options,
    QWidget *parent
)
    : QDialog(parent),
      m_format(format)
{
    setupUi();
    loadOptions(options);
}

void ConversionOptionsDialog::setupUi()
{
    setWindowTitle(
        tr("Opciones %1").arg(formatName())
    );

    setModal(true);

    auto *mainLayout =
        new QVBoxLayout(this);

    mainLayout->setContentsMargins(
        16, 16, 16, 16
    );

    mainLayout->setSpacing(14);

    auto *description =
        new QLabel(
            tr("Opciones de conversión para %1.")
                .arg(formatName()),
            this
        );

    mainLayout->addWidget(description);

    m_pages = new QStackedWidget(this);

    m_pages->addWidget(createJpegPage());
    m_pages->addWidget(createTiffPage());
    m_pages->addWidget(createPsdPage());
    m_pages->addWidget(createPngPage());
    m_pages->addWidget(createWebPPage());
    m_pages->addWidget(createBmpPage());

    switch (m_format) {
    case ImageFormat::JPEG:
        m_pages->setCurrentIndex(0);
        break;

    case ImageFormat::TIFF:
        m_pages->setCurrentIndex(1);
        break;

    case ImageFormat::PSD:
        m_pages->setCurrentIndex(2);
        break;

    case ImageFormat::PNG:
        m_pages->setCurrentIndex(3);
        break;

    case ImageFormat::WebP:
        m_pages->setCurrentIndex(4);
        break;

    case ImageFormat::BMP:
        m_pages->setCurrentIndex(5);
        break;
    }

    mainLayout->addWidget(m_pages);

    auto *buttons =
        new QDialogButtonBox(
            QDialogButtonBox::Cancel |
            QDialogButtonBox::Ok,
            this
        );

    connect(
        buttons,
        &QDialogButtonBox::accepted,
        this,
        &QDialog::accept
    );

    connect(
        buttons,
        &QDialogButtonBox::rejected,
        this,
        &QDialog::reject
    );

    mainLayout->addWidget(buttons);

    setMinimumWidth(380);
}

QWidget *ConversionOptionsDialog::createJpegPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QFormLayout(page);

    m_jpegQualitySpin =
        new QSpinBox(page);

    m_jpegQualitySpin->setRange(0, 100);
    m_jpegQualitySpin->setValue(90);
    m_jpegQualitySpin->setSuffix(tr(" %"));

    layout->addRow(
        tr("Calidad:"),
        m_jpegQualitySpin
    );

    return page;
}

QWidget *ConversionOptionsDialog::createTiffPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QFormLayout(page);

    m_tiffCompressionCombo =
        new QComboBox(page);

    m_tiffCompressionCombo->addItem(
        tr("Ninguna"),
        QStringLiteral("none")
    );

    m_tiffCompressionCombo->addItem(
        tr("LZW"),
        QStringLiteral("lzw")
    );

    m_tiffCompressionCombo->addItem(
        tr("ZIP"),
        QStringLiteral("zip")
    );

    layout->addRow(
        tr("Compresión:"),
        m_tiffCompressionCombo
    );

    return page;
}

QWidget *ConversionOptionsDialog::createPsdPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    m_psdMaximizeCompatibilityCheck =
        new QCheckBox(
            tr("Maximizar compatibilidad"),
            page
        );

    m_psdMaximizeCompatibilityCheck
        ->setChecked(true);

    layout->addWidget(
        m_psdMaximizeCompatibilityCheck
    );

    layout->addStretch();

    return page;
}

QWidget *ConversionOptionsDialog::createPngPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QFormLayout(page);

    m_pngCompressionSpin =
        new QSpinBox(page);

    m_pngCompressionSpin->setRange(0, 9);
    m_pngCompressionSpin->setValue(6);

    layout->addRow(
        tr("Compresión:"),
        m_pngCompressionSpin
    );

    return page;
}

QWidget *ConversionOptionsDialog::createWebPPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QFormLayout(page);

    m_webpQualitySpin =
        new QSpinBox(page);

    m_webpQualitySpin->setRange(0, 100);
    m_webpQualitySpin->setValue(90);
    m_webpQualitySpin->setSuffix(tr(" %"));

    m_webpLosslessCheck =
        new QCheckBox(
            tr("Sin pérdida (Lossless)"),
            page
        );

    layout->addRow(
        tr("Calidad:"),
        m_webpQualitySpin
    );

    layout->addRow(
        QString(),
        m_webpLosslessCheck
    );

    connect(
        m_webpLosslessCheck,
        &QCheckBox::toggled,
        m_webpQualitySpin,
        &QWidget::setDisabled
    );

    return page;
}

QWidget *ConversionOptionsDialog::createBmpPage()
{
    auto *page = new QWidget(this);
    auto *layout = new QVBoxLayout(page);

    auto *label =
        new QLabel(
            tr(
                "BMP no tiene opciones adicionales "
                "de conversión."
            ),
            page
        );

    label->setWordWrap(true);

    layout->addWidget(label);
    layout->addStretch();

    return page;
}

void ConversionOptionsDialog::loadOptions(
    const QVariantMap &options
)
{
    switch (m_format) {
    case ImageFormat::JPEG:
        m_jpegQualitySpin->setValue(
            options.value(
                OptionKeys::Quality,
                90
            ).toInt()
        );
        break;

    case ImageFormat::TIFF: {
        const QString compression =
            options.value(
                OptionKeys::Compression,
                QStringLiteral("lzw")
            ).toString();

        const int index =
            m_tiffCompressionCombo->findData(
                compression
            );

        if (index >= 0) {
            m_tiffCompressionCombo
                ->setCurrentIndex(index);
        }

        break;
    }

    case ImageFormat::PSD:
        m_psdMaximizeCompatibilityCheck
            ->setChecked(
                options.value(
                    OptionKeys::MaximizeCompatibility,
                    true
                ).toBool()
            );
        break;

    case ImageFormat::PNG:
        m_pngCompressionSpin->setValue(
            options.value(
                OptionKeys::Compression,
                6
            ).toInt()
        );
        break;

    case ImageFormat::WebP: {
        const bool lossless =
            options.value(
                OptionKeys::Lossless,
                false
            ).toBool();

        m_webpQualitySpin->setValue(
            options.value(
                OptionKeys::Quality,
                90
            ).toInt()
        );

        m_webpLosslessCheck->setChecked(
            lossless
        );

        m_webpQualitySpin->setDisabled(
            lossless
        );

        break;
    }

    case ImageFormat::BMP:
        break;
    }
}

QVariantMap ConversionOptionsDialog::options() const
{
    QVariantMap result;

    switch (m_format) {
    case ImageFormat::JPEG:
        result.insert(
            OptionKeys::Quality,
            m_jpegQualitySpin->value()
        );
        break;

    case ImageFormat::TIFF:
        result.insert(
            OptionKeys::Compression,
            m_tiffCompressionCombo
                ->currentData()
        );
        break;

    case ImageFormat::PSD:
        result.insert(
            OptionKeys::MaximizeCompatibility,
            m_psdMaximizeCompatibilityCheck
                ->isChecked()
        );
        break;

    case ImageFormat::PNG:
        result.insert(
            OptionKeys::Compression,
            m_pngCompressionSpin->value()
        );
        break;

    case ImageFormat::WebP:
        result.insert(
            OptionKeys::Quality,
            m_webpQualitySpin->value()
        );

        result.insert(
            OptionKeys::Lossless,
            m_webpLosslessCheck->isChecked()
        );
        break;

    case ImageFormat::BMP:
        break;
    }

    return result;
}

QString ConversionOptionsDialog::formatName() const
{
    switch (m_format) {
    case ImageFormat::PSD:
        return QStringLiteral("PSD");

    case ImageFormat::TIFF:
        return QStringLiteral("TIFF");

    case ImageFormat::JPEG:
        return QStringLiteral("JPEG");

    case ImageFormat::PNG:
        return QStringLiteral("PNG");

    case ImageFormat::WebP:
        return QStringLiteral("WebP");

    case ImageFormat::BMP:
        return QStringLiteral("BMP");
    }

    return {};
}