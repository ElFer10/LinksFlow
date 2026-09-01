#pragma once

#include <QWidget>

#include "../domain/preset.h"

class ResolutionTab;
class SummaryPanel;

class ConfigurationPage : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigurationPage(QWidget *parent = nullptr);

signals:
    void analyzeDocumentRequested();
    void helpRequested();
    void exitRequested();

private:
    void setupUi();
    void setupConnections();

    void updatePresetFromUi();
    void updateUiFromPreset();
    void resetPreset();

private:
    Preset m_preset;

    ResolutionTab *m_resolutionTab = nullptr;
    SummaryPanel *m_summaryPanel = nullptr;
};
