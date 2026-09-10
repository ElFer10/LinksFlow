#include "mainwindow.h"

#include "services/adobebridgetransport.h"
#include "services/adobeindesignbridge.h"
#include "services/adobephotoshopbridge.h"
#include "services/backupservice.h"
#include "services/indesignbridge.h"
#include "services/processingcontroller.h"

#include "ui/analysis/analysispage.h"
#include "ui/configurationpage.h"
#include "ui/processing/processingdialog.h"

#include <QAbstractButton>
#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QStatusBar>
#include <QString>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    constexpr int winWidth{1200}, winHeight{760};
    constexpr int minWinWidth{900}, minWinHeight{600};

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

    // Cerramos y destruimos el transporte mientras MainWindow todavía existe.

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
    m_pages = new QStackedWidget(this);

    m_adobeTransport = new AdobeBridgeTransport(this);

    m_indesignBridge = new AdobeInDesignBridge(m_adobeTransport, this);
    m_photoshopBridge = new AdobePhotoshopBridge(m_adobeTransport, this);

    m_indesignConnectionLabel = new QLabel(this);
    m_photoshopConnectionLabel = new QLabel(this);

    statusBar()->addPermanentWidget(m_indesignConnectionLabel);
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
                m_photoshopBridge->ping();
            break;
        case AdobeHost::Unknown:
            break;
        }
    });

    // Iniciar servidor WebSocket
    if (!m_adobeTransport->start(17321))
        qWarning() << "No se pudo iniciar Adobe Bridge";

    updateInDesignConnectionState(m_adobeTransport->hasClient(AdobeHost::InDesign));
    updatePhotoshopConnectionState(m_adobeTransport->hasClient(AdobeHost::Photoshop));

    m_processingController = new ProcessingController(this);

    // Páginas
    m_configurationPage = new ConfigurationPage(m_pages);

    m_pages->addWidget(m_configurationPage);

    m_analysisPage = new AnalysisPage(m_pages);

    m_pages->addWidget(m_analysisPage);

    setCentralWidget(m_pages);

    // Configuración → salir
    connect(m_configurationPage, &ConfigurationPage::exitRequested, qApp, &QApplication::quit);

    // Análisis → volver
    connect(m_analysisPage, &AnalysisPage::backRequested, this,
            [this]() { m_pages->setCurrentWidget(m_configurationPage); });

    // Análisis → salir
    connect(m_analysisPage, &AnalysisPage::exitRequested, qApp, &QApplication::quit);

    // Solicitar análisis a InDesign
    connect(m_configurationPage, &ConfigurationPage::analyzeDocumentRequested, this,
            [this]() { m_indesignBridge->analyzeActiveDocument(); });

    // Procesamiento
    connect(m_analysisPage, &AnalysisPage::processRequested, this, [this]() {
        const QList<LinkInfo> links = m_analysisPage->links();

        const OptimizationSettings settings = m_configurationPage->currentSettings();

        const QList<ProcessingJob> jobs = m_processingController->createJobs(links, settings);

        if (jobs.isEmpty())
        {
            return;
        }

        //
        // Preguntar si se desea crear backup.
        //

        QMessageBox backupQuestion(this);

        backupQuestion.setWindowTitle(tr("Copia de seguridad"));

        backupQuestion.setIcon(QMessageBox::Question);

        backupQuestion.setText(tr("LinksFlow va a procesar %1 archivo(s).").arg(jobs.size()));

        backupQuestion.setInformativeText(tr("¿Quieres crear una copia de seguridad "
                                             "de los archivos originales antes de "
                                             "procesarlos?"));

        QPushButton *backupButton = backupQuestion.addButton(tr("Crear copia"), QMessageBox::AcceptRole);

        QPushButton *withoutBackupButton = backupQuestion.addButton(tr("Sin copia"), QMessageBox::DestructiveRole);

        QPushButton *cancelButton = backupQuestion.addButton(tr("Cancelar"), QMessageBox::RejectRole);

        backupQuestion.setDefaultButton(backupButton);

        backupQuestion.exec();

        QAbstractButton *clickedButton = backupQuestion.clickedButton();

        //
        // Cancelar.
        //

        if (clickedButton == static_cast<QAbstractButton *>(cancelButton))
        {
            return;
        }

        //
        // Crear copia de seguridad.
        //

        if (clickedButton == static_cast<QAbstractButton *>(backupButton))
        {
            if (m_currentDocument.path.isEmpty())
            {
                QMessageBox::critical(this, tr("Copia de seguridad"),
                                      tr("No se puede crear la copia de "
                                         "seguridad porque no se conoce "
                                         "la ubicación del documento "
                                         "de InDesign."));

                return;
            }

            const BackupResult backupResult = m_backupService->createBackup(jobs, m_currentDocument.path);

            if (!backupResult.success)
            {

                QMessageBox::critical(this,
                                      tr("Error al crear la copia "
                                         "de seguridad"),
                                      backupResult.errorMessage);

                //
                // Si el usuario pidió backup
                // y este falla, no procesamos.
                //

                return;
            }

            QMessageBox::information(this, tr("Copia de seguridad creada"),
                                     tr("Se copiaron %1 archivo(s) en:\n\n%2")
                                         .arg(backupResult.copiedFiles)
                                         .arg(backupResult.backupDirectory));
        }

        //
        // Si no pulsó ni "Crear copia"
        // ni "Sin copia", no continuar.
        //

        if (clickedButton != static_cast<QAbstractButton *>(backupButton) &&
            clickedButton != static_cast<QAbstractButton *>(withoutBackupButton))
        {
            return;
        }

        //
        // Crear modal de procesamiento.
        //

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
    // Resultado de análisis
    connect(m_indesignBridge, &InDesignBridge::analysisCompleted, this, [this](const InDesignDocumentInfo &document) {
        m_currentDocument = document;

        m_analysisPage->setLinks(document.links);
        for (const LinkInfo &link : document.links)
            m_pages->setCurrentWidget(m_analysisPage);

        qDebug() << "Documento InDesign:" << document.name;

        qDebug() << "Ruta documento:" << document.path;
    });

    // Error de análisis
    connect(m_indesignBridge, &InDesignBridge::analysisFailed, this,
            [this](const QString &message) { QMessageBox::critical(this, tr("Error de análisis"), message); });

    // Diagnóstico de conexiones Adobe
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

    connect(m_photoshopBridge, &AdobePhotoshopBridge::imageInspected, this, [](const PhotoshopImageInfo &info) {
        qDebug() << "Photoshop inspect:" << info.name << info.width << "x" << info.height << "@" << info.resolution
                 << "ppi"
                 << "mode:" << info.mode << "layers:" << info.layerCount;
    });

    connect(m_photoshopBridge, &AdobePhotoshopBridge::imageInspectionFailed, this,
            [](const QString &message) { qWarning() << "Photoshop inspect error:" << message; });
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
