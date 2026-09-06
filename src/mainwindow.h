#pragma once
#include "services/indesignbridge.h"
#include <QLabel>
#include <QMainWindow>

class QStackedWidget;
class AnalysisPage;
class InDesignBridge;
class AdobeBridgeTransport;

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
    QLabel *m_indesignConnectionLabel = nullptr;

    void createInterface();
    void updateInDesignConnectionState(bool connected);
};
