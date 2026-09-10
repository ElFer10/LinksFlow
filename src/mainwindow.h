#pragma once
#include "domain/indesigndocumentinfo.h"
#include "services/indesignbridge.h"
#include "ui/configurationpage.h"
#include <QLabel>
#include <QMainWindow>

class QStackedWidget;
class AnalysisPage;
class InDesignBridge;
class AdobeBridgeTransport;
class ProcessingController;
class ConfigurationPage;
class AdobePhotoshopBridge;
class BackupService;

class MainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

  private:
    QStackedWidget *m_pages = nullptr;
    AnalysisPage *m_analysisPage = nullptr;
    InDesignBridge *m_indesignBridge = nullptr;
    AdobeBridgeTransport *m_adobeTransport = nullptr;
    ProcessingController *m_processingController = nullptr;
    ConfigurationPage *m_configurationPage = nullptr;
    QLabel *m_indesignConnectionLabel = nullptr;
    QLabel *m_photoshopConnectionLabel = nullptr;
    AdobePhotoshopBridge *m_photoshopBridge = nullptr;
    InDesignDocumentInfo m_currentDocument;
    BackupService *m_backupService = nullptr;

    void createInterface();
    void updateInDesignConnectionState(bool connected);
    void updatePhotoshopConnectionState(bool connected);
};
