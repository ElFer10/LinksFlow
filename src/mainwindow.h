#pragma once
#include "services/indesignbridge.h"
#include <QMainWindow>

class QStackedWidget;
class AnalysisPage;
class InDesignBridge;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override = default;

private:
  void createInterface();

  QStackedWidget *m_pages = nullptr;
  AnalysisPage *m_analysisPage = nullptr;
  InDesignBridge *m_indesignBridge = nullptr;
};
