
#pragma once

#include <QFrame>

#include "../../domain/preset.h"

class QLabel;
class QPushButton;

class SummaryPanel : public QFrame
{
    Q_OBJECT

public:
    explicit SummaryPanel(QWidget *parent = nullptr);

    void setPreset(const Preset &preset);

signals:
    void resetRequested();

private:
    void setupUi();

    QString resolutionUnitText(
        ResolutionUnit unit
    ) const;

private:
    QLabel *m_presetValue = nullptr;

    QLabel *m_colorResolutionValue = nullptr;
    QLabel *m_monochromeResolutionValue = nullptr;

    QLabel *m_optimizationMethodValue = nullptr;
    QLabel *m_cropValue = nullptr;
    QLabel *m_safetyAreaValue = nullptr;

    QLabel *m_editingValue = nullptr;
    QLabel *m_conversionValue = nullptr;

    QPushButton *m_resetButton = nullptr;
};
