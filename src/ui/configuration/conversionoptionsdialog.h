#pragma once

#include <QDialog>
#include <QVariantMap>

#include "../../domain/conversionsettings.h"

class QCheckBox;
class QComboBox;
class QSpinBox;
class QStackedWidget;

class ConversionOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ConversionOptionsDialog(ImageFormat format, const QVariantMap &options, QWidget *parent = nullptr);

    QVariantMap options() const;

    void setRuleOptions(int row, const QVariantMap &options);

private:
    void setupUi();
    void loadOptions(const QVariantMap &options);

    QWidget *createJpegPage();
    QWidget *createTiffPage();
    QWidget *createPsdPage();
    QWidget *createPngPage();
    QWidget *createWebPPage();
    QWidget *createBmpPage();

    QString formatName() const;

private:
    ImageFormat m_format;

    QStackedWidget *m_pages = nullptr;

    QSpinBox *m_jpegQualitySpin = nullptr;

    QComboBox *m_tiffCompressionCombo = nullptr;

    QCheckBox *m_psdMaximizeCompatibilityCheck = nullptr;

    QSpinBox *m_pngCompressionSpin = nullptr;

    QSpinBox *m_webpQualitySpin = nullptr;
    QCheckBox *m_webpLosslessCheck = nullptr;
};
