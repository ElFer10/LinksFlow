#include "mainwindow.h"
#include "services/indesignbridge.h"
// #include "services/mockindesignbridge.h"
#include "services/adobeindesignbridge.h"
#include "ui/analysis/analysispage.h"
#include "ui/configurationpage.h"

#include <QApplication>
#include <QMessageBox>
#include <QStackedWidget>
#include <QString>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  constexpr int winWidth{1200}, winHeight{760}, minWinHeight{600},
      minWinWidth{900};
  const QString appName{"LinksFlow"};

  createInterface();

  resize(winWidth, winHeight);
  setMinimumSize(minWinWidth, minWinHeight);

  setWindowTitle(appName);
}

void MainWindow::createInterface() {
  m_pages = new QStackedWidget(this);
  // m_indesignBridge = new MockInDesignBridge(this);
  auto *bridge = new AdobeInDesignBridge(this);

  bridge->setAnalysisFilePath(QStringLiteral("/Users/fernando/Desktop/"
                                             "LinksFlow-analysis-v1.json"));

  m_indesignBridge = bridge;
  auto *configurationPage = new ConfigurationPage(m_pages);

  m_pages->addWidget(configurationPage);

  m_analysisPage = new AnalysisPage(m_pages);

  m_pages->addWidget(m_analysisPage);

  setCentralWidget(m_pages);

  connect(configurationPage, &ConfigurationPage::exitRequested, qApp,
          &QApplication::quit);
  connect(m_analysisPage, &AnalysisPage::backRequested, this,
          [this, configurationPage]() {
            m_pages->setCurrentWidget(configurationPage);
          });

  connect(m_analysisPage, &AnalysisPage::exitRequested, qApp,
          &QApplication::quit);

  connect(configurationPage, &ConfigurationPage::analyzeDocumentRequested, this,
          [this]() { m_indesignBridge->analyzeActiveDocument(); });

  connect(m_indesignBridge, &InDesignBridge::analysisCompleted, this,
          [this](const QList<LinkInfo> &links) {
            m_analysisPage->setLinks(links);

            m_pages->setCurrentWidget(m_analysisPage);
          });
  connect(m_indesignBridge, &InDesignBridge::analysisFailed, this,
          [this](const QString &message) {
            QMessageBox::critical(this, tr("Error de análisis"), message);
          });
}
