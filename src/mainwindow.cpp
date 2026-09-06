#include "mainwindow.h"
#include "services/adobeindesignbridge.h"
#include "services/indesignbridge.h"
#include "ui/analysis/analysispage.h"
#include "ui/configurationpage.h"

#include "services/adobebridgetransport.h"
#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QMessageBox>
#include <QStackedWidget>
#include <QStatusBar>
#include <QString>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    constexpr int winWidth{1200}, winHeight{760}, minWinHeight{600}, minWinWidth{900};
    const QString appName{"LinksFlow"};

    createInterface();

    resize(winWidth, winHeight);
    setMinimumSize(minWinWidth, minWinHeight);

    setWindowTitle(appName);
}

MainWindow::~MainWindow()
{
    if (m_adobeTransport)
        disconnect(m_adobeTransport, nullptr, this, nullptr);

    // Destruir primero el consumidor del transporte.
    delete m_indesignBridge;
    m_indesignBridge = nullptr;

    // Cerrar y destruir el transporte mientras MainWindow todavía conserva su tipo dinámico.
    if (m_adobeTransport)
    {
        m_adobeTransport->stop();
        delete m_adobeTransport;
        m_adobeTransport = nullptr;
    }

    m_indesignConnectionLabel = nullptr;
}

void MainWindow::createInterface()
{
    m_pages = new QStackedWidget(this);

    m_adobeTransport = new AdobeBridgeTransport(this);
    m_indesignConnectionLabel = new QLabel(this);

    statusBar()->addPermanentWidget(m_indesignConnectionLabel);

    connect(m_adobeTransport, &AdobeBridgeTransport::connectionChanged, this,
            &MainWindow::updateInDesignConnectionState);

    if (!m_adobeTransport->start(17321))
        qWarning() << "No se pudo iniciar Adobe Bridge";

    m_indesignBridge = new AdobeInDesignBridge(m_adobeTransport, this);

    auto *configurationPage = new ConfigurationPage(m_pages);

    m_pages->addWidget(configurationPage);

    m_analysisPage = new AnalysisPage(m_pages);

    m_pages->addWidget(m_analysisPage);

    setCentralWidget(m_pages);

    connect(configurationPage, &ConfigurationPage::exitRequested, qApp, &QApplication::quit);
    connect(m_analysisPage, &AnalysisPage::backRequested, this,
            [this, configurationPage]() { m_pages->setCurrentWidget(configurationPage); });

    connect(m_analysisPage, &AnalysisPage::exitRequested, qApp, &QApplication::quit);

    connect(configurationPage, &ConfigurationPage::analyzeDocumentRequested, this,
            [this]() { m_indesignBridge->analyzeActiveDocument(); });

    connect(m_indesignBridge, &InDesignBridge::analysisCompleted, this, [this](const QList<LinkInfo> &links) {
        m_analysisPage->setLinks(links);
        m_pages->setCurrentWidget(m_analysisPage);
    });

    connect(m_indesignBridge, &InDesignBridge::analysisFailed, this,
            [this](const QString &message) { QMessageBox::critical(this, tr("Error de análisis"), message); });

    connect(m_adobeTransport, &AdobeBridgeTransport::clientConnected, this,
            [this]() { qDebug() << "Adobe Bridge conectado"; });

    connect(m_adobeTransport, &AdobeBridgeTransport::clientDisconnected, this,
            []() { qDebug() << "Adobe Bridge desconectado"; });

    connect(m_adobeTransport, &AdobeBridgeTransport::textMessageReceived, this,
            [](const QString &message) { qDebug() << "Mensaje Adobe:" << message; });

    if (!m_adobeTransport->start(17321))
        qWarning() << "No se pudo iniciar Adobe Bridge";
}

void MainWindow::updateInDesignConnectionState(bool connected)
{
    if (!m_indesignConnectionLabel)
        return;

    if (connected)
    {
        m_indesignConnectionLabel->setText(QStringLiteral("<span style=\"color:#34C759;\">●</span> "
                                                          "InDesign conectado"));

        m_indesignConnectionLabel->setToolTip(QStringLiteral("LinksFlow está conectado con Adobe InDesign."));
    }
    else
    {
        m_indesignConnectionLabel->setText(QStringLiteral("<span style=\"color:#FF3B30;\">●</span> "
                                                          "InDesign desconectado"));

        m_indesignConnectionLabel->setToolTip(QStringLiteral("LinksFlow no está conectado con Adobe InDesign."));
    }
}
