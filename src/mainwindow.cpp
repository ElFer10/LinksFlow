#include "mainwindow.h"

#include "services/adobebridgetransport.h"
#include "services/adobeindesignbridge.h"
#include "services/adobephotoshopbridge.h"
#include "services/indesignbridge.h"
#include "services/processingcontroller.h"

#include "ui/analysis/analysispage.h"
#include "ui/configurationpage.h"
#include "ui/processing/processingdialog.h"

#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QMessageBox>
#include <QStackedWidget>
#include <QStatusBar>
#include <QString>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    constexpr int winWidth = 1200;
    constexpr int winHeight = 760;
    constexpr int minWinWidth = 900;
    constexpr int minWinHeight = 600;

    const QString appName = QStringLiteral("LinksFlow");

    createInterface();

    resize(winWidth, winHeight);

    setMinimumSize(minWinWidth, minWinHeight);

    setWindowTitle(appName);
}

MainWindow::~MainWindow()
{
    // Evitamos señales tardías durante la destrucción de MainWindow.

    if (m_adobeTransport)
        disconnect(m_adobeTransport, nullptr, this, nullptr);

    // Destruimos primero el consumidor del transporte.

    delete m_indesignBridge;
    m_indesignBridge = nullptr;

    //
    // Cerramos y destruimos el transporte mientras MainWindow todavía existe.
    //

    if (m_adobeTransport)
    {
        m_adobeTransport->stop();

        delete m_adobeTransport;
        m_adobeTransport = nullptr;
    }

    m_indesignConnectionLabel = nullptr;
    m_photoshopConnectionLabel = nullptr;
}

void MainWindow::createInterface()
{
    // Contenedor principal

    m_pages = new QStackedWidget(this);

    // Adobe Bridge

    m_adobeTransport = new AdobeBridgeTransport(this);

    // Bridge InDesign

    m_indesignBridge = new AdobeInDesignBridge(m_adobeTransport, this);
    m_photoshopBridge = new AdobePhotoshopBridge(m_adobeTransport, this);

    // Estado InDesign

    m_indesignConnectionLabel = new QLabel(this);

    statusBar()->addPermanentWidget(m_indesignConnectionLabel);
    m_photoshopConnectionLabel = new QLabel(this);

    statusBar()->addPermanentWidget(m_photoshopConnectionLabel);

    // Connection Changeged

    connect(m_adobeTransport, &AdobeBridgeTransport::connectionChanged, this, [this](AdobeHost host, bool connected) {
        switch (host)
        {

        case AdobeHost::InDesign:

            updateInDesignConnectionState(connected);

            break;
        case AdobeHost::Photoshop:

            updatePhotoshopConnectionState(connected);

            if (connected && m_photoshopBridge)
            {
                m_photoshopBridge->ping();
            }

            break;
        case AdobeHost::Unknown:
            break;
        }
    });

    //
    // Iniciar servidor WebSocket
    //

    if (!m_adobeTransport->start(17321))
    {
        qWarning() << "No se pudo iniciar Adobe Bridge";
    }

    //
    // Estado inicial.
    //
    // En este momento normalmente será false, hasta que el plugin UXP haga handshake.
    //

    updateInDesignConnectionState(m_adobeTransport->hasClient(AdobeHost::InDesign));
    updatePhotoshopConnectionState(m_adobeTransport->hasClient(AdobeHost::Photoshop));

    //
    // Procesamiento
    //

    m_processingController = new ProcessingController(this);

    //
    // Páginas
    //

    m_configurationPage = new ConfigurationPage(m_pages);

    m_pages->addWidget(m_configurationPage);

    m_analysisPage = new AnalysisPage(m_pages);

    m_pages->addWidget(m_analysisPage);

    setCentralWidget(m_pages);

    //
    // Configuración → salir
    //

    connect(m_configurationPage, &ConfigurationPage::exitRequested, qApp, &QApplication::quit);

    //
    // Análisis → volver
    //

    connect(m_analysisPage, &AnalysisPage::backRequested, this,
            [this]() { m_pages->setCurrentWidget(m_configurationPage); });

    //
    // Análisis → salir
    //

    connect(m_analysisPage, &AnalysisPage::exitRequested, qApp, &QApplication::quit);

    //
    // Solicitar análisis a InDesign
    //

    connect(m_configurationPage, &ConfigurationPage::analyzeDocumentRequested, this,
            [this]() { m_indesignBridge->analyzeActiveDocument(); });

    //
    // Procesamiento
    //

    connect(m_analysisPage, &AnalysisPage::processRequested, this, [this]() {
        const QList<LinkInfo> links = m_analysisPage->links();

        const OptimizationSettings settings = m_configurationPage->currentSettings();

        const QList<ProcessingJob> jobs = m_processingController->createJobs(links, settings);

        if (jobs.isEmpty())
            return;

        auto *dialog = new ProcessingDialog(this);

        dialog->setAttribute(Qt::WA_DeleteOnClose);

        dialog->setJobs(jobs);

        connect(m_processingController, &ProcessingController::jobStarted, dialog, &ProcessingDialog::jobStarted);

        connect(m_processingController, &ProcessingController::jobCompleted, dialog, &ProcessingDialog::jobCompleted);

        connect(m_processingController, &ProcessingController::processingCompleted, dialog,
                &ProcessingDialog::processingCompleted);

        connect(dialog, &ProcessingDialog::cancelRequested, m_processingController,
                &ProcessingController::cancelProcessing);

        dialog->show();

        m_processingController->processJobs(jobs);
    });

    //
    // Resultado de análisis
    //

    connect(m_indesignBridge, &InDesignBridge::analysisCompleted, this, [this](const QList<LinkInfo> &links) {
        m_analysisPage->setLinks(links);

        m_pages->setCurrentWidget(m_analysisPage);
    });

    //
    // Error de análisis
    //

    connect(m_indesignBridge, &InDesignBridge::analysisFailed, this,
            [this](const QString &message) { QMessageBox::critical(this, tr("Error de análisis"), message); });

    //
    // Diagnóstico de conexiones Adobe
    //

    connect(m_adobeTransport, &AdobeBridgeTransport::clientConnected, this, [](AdobeHost host) {
        QString hostName;

        switch (host)
        {

        case AdobeHost::InDesign:
            hostName = QStringLiteral("InDesign");
            break;

        case AdobeHost::Photoshop:
            hostName = QStringLiteral("Photoshop");
            break;

        case AdobeHost::Unknown:
            hostName = QStringLiteral("Unknown");
            break;
        }

        qDebug() << "Adobe Bridge conectado:" << hostName;
    });

    connect(m_adobeTransport, &AdobeBridgeTransport::clientDisconnected, this, [](AdobeHost host) {
        QString hostName;

        switch (host)
        {

        case AdobeHost::InDesign:
            hostName = QStringLiteral("InDesign");
            break;

        case AdobeHost::Photoshop:
            hostName = QStringLiteral("Photoshop");
            break;

        case AdobeHost::Unknown:
            hostName = QStringLiteral("Unknown");
            break;
        }

        qDebug() << "Adobe Bridge desconectado:" << hostName;
    });

    connect(m_adobeTransport, &AdobeBridgeTransport::textMessageReceived, this,
            [](AdobeHost host, const QString &message) {
                QString hostName;

                switch (host)
                {

                case AdobeHost::InDesign:
                    hostName = QStringLiteral("InDesign");
                    break;

                case AdobeHost::Photoshop:
                    hostName = QStringLiteral("Photoshop");
                    break;

                case AdobeHost::Unknown:
                    hostName = QStringLiteral("Unknown");
                    break;
                }

                qDebug() << "Mensaje Adobe" << hostName << ":" << message;
            });

    // Photoshop PingPong
    connect(m_photoshopBridge, &AdobePhotoshopBridge::pingSucceeded, this,
            []() { qDebug() << "Photoshop ping: pong"; });

    connect(m_photoshopBridge, &AdobePhotoshopBridge::pingFailed, this,
            [](const QString &message) { qWarning() << "Photoshop ping error:" << message; });
}

void MainWindow::updateInDesignConnectionState(bool connected)
{
    if (!m_indesignConnectionLabel)
        return;

    if (connected)
    {
        m_indesignConnectionLabel->setText(
            QStringLiteral("<span style=\"color:#34C759;\">●</span> InDesign conectado"));

        m_indesignConnectionLabel->setToolTip(QStringLiteral("LinksFlow está conectado con Adobe InDesign."));
    }
    else
    {

        m_indesignConnectionLabel->setText(
            QStringLiteral("<span style=\"color:#FF3B30;\">● </span> InDesign desconectado"));

        m_indesignConnectionLabel->setToolTip(QStringLiteral("LinksFlow no está conectado con Adobe InDesign."));
    }
}

void MainWindow::updatePhotoshopConnectionState(bool connected)
{
    if (!m_photoshopConnectionLabel)
        return;

    if (connected)
    {

        m_photoshopConnectionLabel->setText(
            QStringLiteral("<span style=\"color:#34C759;\"> ●</span> Photoshop conectado"));
        m_photoshopConnectionLabel->setToolTip(QStringLiteral("LinksFlow está conectado con Adobe Photoshop."));
    }
    else
    {

        m_photoshopConnectionLabel->setText(
            QStringLiteral("<span style=\"color:#FF3B30;\"> ● </span> Photoshop desconectado"));
        m_photoshopConnectionLabel->setToolTip(QStringLiteral("LinksFlow no está conectado con Adobe Photoshop."));
    }
}
