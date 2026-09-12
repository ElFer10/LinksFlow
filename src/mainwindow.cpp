#include "mainwindow.h"

#include "domain/linkupdateresult.h"

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
#include <QtGlobal>

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
    if (m_adobeTransport)
        disconnect(m_adobeTransport, nullptr, this, nullptr);

    delete m_indesignBridge;
    m_indesignBridge = nullptr;

    delete m_photoshopBridge;
    m_photoshopBridge = nullptr;

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
    // ========================================================
    // CONTENEDOR PRINCIPAL
    // ========================================================

    m_pages = new QStackedWidget(this);

    // ========================================================
    // ADOBE BRIDGE
    // ========================================================

    m_adobeTransport = new AdobeBridgeTransport(this);

    m_indesignBridge = new AdobeInDesignBridge(m_adobeTransport, this);

    m_photoshopBridge = new AdobePhotoshopBridge(m_adobeTransport, this);

    // ========================================================
    // STATUS BAR
    // ========================================================

    m_indesignConnectionLabel = new QLabel(this);

    m_photoshopConnectionLabel = new QLabel(this);

    statusBar()->addPermanentWidget(m_indesignConnectionLabel);

    statusBar()->addPermanentWidget(m_photoshopConnectionLabel);

    // Cambio de conexión Adobe
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

    // ========================================================
    // INICIAR WEBSOCKET SERVER
    // ========================================================
    if (!m_adobeTransport->start(17321))
    {
        qWarning() << "No se pudo iniciar Adobe Bridge";
    }

    // Estado inicial
    updateInDesignConnectionState(m_adobeTransport->hasClient(AdobeHost::InDesign));

    updatePhotoshopConnectionState(m_adobeTransport->hasClient(AdobeHost::Photoshop));

    // ========================================================
    // SERVICIOS
    // ========================================================

    m_processingController = new ProcessingController(m_photoshopBridge, m_indesignBridge, this);
    m_backupService = new BackupService(this);

    // ========================================================
    // PÁGINAS
    // ========================================================

    m_configurationPage = new ConfigurationPage(m_pages);
    m_analysisPage = new AnalysisPage(m_pages);

    m_pages->addWidget(m_configurationPage);
    m_pages->addWidget(m_analysisPage);

    setCentralWidget(m_pages);

    // ========================================================
    // CONFIGURACIÓN
    // ========================================================

    // Salir
    connect(m_configurationPage, &ConfigurationPage::exitRequested, qApp, &QApplication::quit);

    // Solicitar análisis a InDesign
    connect(m_configurationPage, &ConfigurationPage::analyzeDocumentRequested, this,
            [this]() { m_indesignBridge->analyzeActiveDocument(); });

    // ========================================================
    // ANALYSIS PAGE
    // ========================================================

    // Volver
    connect(m_analysisPage, &AnalysisPage::backRequested, this,
            [this]() { m_pages->setCurrentWidget(m_configurationPage); });

    // Salir
    connect(m_analysisPage, &AnalysisPage::exitRequested, qApp, &QApplication::quit);

    // ========================================================
    // PROCESAR
    // ========================================================
    //
    // IMPORTANTE:
    //
    // Esta es todavía una fase de prueba.
    //
    // Se procesa solamente UNA imagen real mediante Photoshop.
    //
    // ProcessingController::processJobs() NO se llama todavía.
    //
    // ========================================================
    //

    connect(m_analysisPage, &AnalysisPage::processRequested, this, [this]() {
        // Crear jobs según selección y configuración actual.
        const QList<LinkInfo> links = m_analysisPage->links();

        const OptimizationSettings settings = m_configurationPage->currentSettings();

        const QList<ProcessingJob> jobs = m_processingController->createJobs(links, settings);

        if (jobs.isEmpty())
        {
            QMessageBox::information(this, tr("Procesamiento"), tr("No hay archivos seleccionados para procesar."));

            return;
        }

        //
        // =================================================
        // PREGUNTAR POR BACKUP
        // =================================================
        //

        QMessageBox backupQuestion(this);

        backupQuestion.setWindowTitle(tr("Copia de seguridad"));

        backupQuestion.setIcon(QMessageBox::Question);

        backupQuestion.setText(tr("LinksFlow va a procesar %1 archivo(s).").arg(jobs.size()));

        backupQuestion.setInformativeText(
            tr("¿Quieres crear una copia de seguridad de los archivos originales antes de procesarlos?"));

        QPushButton *backupButton = backupQuestion.addButton(tr("Crear copia"), QMessageBox::AcceptRole);

        QPushButton *withoutBackupButton = backupQuestion.addButton(tr("Sin copia"), QMessageBox::DestructiveRole);

        QPushButton *cancelButton = backupQuestion.addButton(tr("Cancelar"), QMessageBox::RejectRole);

        backupQuestion.setDefaultButton(backupButton);

        backupQuestion.exec();

        QAbstractButton *clickedButton = backupQuestion.clickedButton();

        // Cancelar
        if (clickedButton == cancelButton)
            return;

        bool backupCreated = false;

        if (!m_photoshopBridge || !m_photoshopBridge->isConnected())
        {
            QMessageBox::critical(this, tr("Photoshop no disponible"),
                                  tr("LinksFlow no está conectado con Adobe Photoshop."));

            return;
        }

        // =================================================
        // CREAR BACKUP
        // =================================================

        if (clickedButton == backupButton)
        {
            if (m_currentDocument.path.isEmpty())
            {
                QMessageBox::critical(this, tr("Copia de seguridad"),
                                      tr("No se puede crear la copia de seguridad porque no se "
                                         "conoce la ubicación del documento de InDesign."));

                return;
            }

            const BackupResult backupResult = m_backupService->createBackup(jobs, m_currentDocument.path);

            if (!backupResult.success)
            {
                QMessageBox::critical(this, tr("Error al crear la copia de seguridad"), backupResult.errorMessage);

                // El usuario pidió backup, por tanto NO procesamos si falla.
                return;
            }

            backupCreated = true;

            QMessageBox::information(this, tr("Copia de seguridad creada"),
                                     tr("Se copiaron %1 archivo(s) en:\n\n%2")
                                         .arg(backupResult.copiedFiles)
                                         .arg(backupResult.backupDirectory));
        }

        // Si no fue ninguna de las opciones válidas, no continuar.

        if (clickedButton != backupButton && clickedButton != withoutBackupButton)
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
    // ========================================================
    // RESULTADO DEL ANÁLISIS DE INDESIGN
    // ========================================================
    //

    connect(m_indesignBridge, &InDesignBridge::analysisCompleted, this, [this](const InDesignDocumentInfo &document) {
        m_currentDocument = document;
        m_analysisPage->setLinks(document.links);
        m_pages->setCurrentWidget(m_analysisPage);
    });

    //
    // Error de análisis
    //

    connect(m_indesignBridge, &InDesignBridge::analysisFailed, this,
            [this](const QString &message) { QMessageBox::critical(this, tr("Error de análisis"), message); });

    //
    // ========================================================
    // DIAGNÓSTICO DE CONEXIONES ADOBE
    // ========================================================
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

    //
    // ========================================================
    // PHOTOSHOP PING
    // ========================================================
    //

    connect(m_photoshopBridge, &AdobePhotoshopBridge::pingSucceeded, this,
            []() { qDebug() << "Photoshop ping: pong"; });

    connect(m_photoshopBridge, &AdobePhotoshopBridge::pingFailed, this,
            [](const QString &message) { qWarning() << "Photoshop ping error:" << message; });

    //
    // ========================================================
    // PHOTOSHOP INSPECT
    // ========================================================
    //

    connect(m_photoshopBridge, &AdobePhotoshopBridge::imageInspected, this, [](const PhotoshopImageInfo &info) {
        qDebug() << "Photoshop inspect:" << info.name << info.width << "x" << info.height << "@" << info.resolution
                 << "ppi"
                 << "mode:" << info.mode << "layers:" << info.layerCount;
    });

    connect(m_photoshopBridge, &AdobePhotoshopBridge::imageInspectionFailed, this,
            [](const QString &message) { qWarning() << "Photoshop inspect error:" << message; });

    //
    // ========================================================
    // PHOTOSHOP RESOLUTION PROCESSING
    // ========================================================
    //

    connect(m_photoshopBridge, &AdobePhotoshopBridge::resolutionProcessed, this,
            [this](const PhotoshopProcessResult &result) {
                qDebug() << "Photoshop resize:" << result.originalWidth << "x" << result.originalHeight << "@"
                         << result.originalResolution << "ppi"
                         << "->" << result.processedWidth << "x" << result.processedHeight << "@"
                         << result.processedResolution << "ppi";

                QMessageBox::information(this, tr("Prueba completada"),
                                         tr("Photoshop procesó correctamente "
                                            "la imagen."
                                            "\n\n"
                                            "Original:"
                                            "\n"
                                            "%1 × %2 px @ %3 ppi"
                                            "\n\n"
                                            "Resultado:"
                                            "\n"
                                            "%4 × %5 px @ %6 ppi")
                                             .arg(result.originalWidth, 0, 'f', 0)
                                             .arg(result.originalHeight, 0, 'f', 0)
                                             .arg(result.originalResolution, 0, 'f', 2)
                                             .arg(result.processedWidth, 0, 'f', 0)
                                             .arg(result.processedHeight, 0, 'f', 0)
                                             .arg(result.processedResolution, 0, 'f', 2));
            });

    connect(m_photoshopBridge, &AdobePhotoshopBridge::resolutionProcessingFailed, this, [this](const QString &message) {
        qWarning() << "Photoshop resize error:" << message;

        QMessageBox::critical(this, tr("Error de Photoshop"), message);
    });

    connect(m_indesignBridge, &InDesignBridge::linksUpdated, this, [](const LinksUpdateResult &result) {
        qDebug() << "Links actualizados en InDesign:" << result.updated << "de" << result.requested;
    });

    connect(m_indesignBridge, &InDesignBridge::linksUpdateFailed, this,
            [](const QString &message) { qWarning() << "Error actualizando links:" << message; });
}

void MainWindow::updateInDesignConnectionState(bool connected)
{
    if (!m_indesignConnectionLabel)
    {
        return;
    }

    if (connected)
    {
        m_indesignConnectionLabel->setText(QStringLiteral("<span style=\"color:#34C759;\">"
                                                          "●"
                                                          "</span> "
                                                          "InDesign conectado"));

        m_indesignConnectionLabel->setToolTip(QStringLiteral("LinksFlow está conectado "
                                                             "con Adobe InDesign."));
    }
    else
    {
        m_indesignConnectionLabel->setText(QStringLiteral("<span style=\"color:#FF3B30;\">"
                                                          "●"
                                                          "</span> "
                                                          "InDesign desconectado"));

        m_indesignConnectionLabel->setToolTip(QStringLiteral("LinksFlow no está conectado "
                                                             "con Adobe InDesign."));
    }
}

void MainWindow::updatePhotoshopConnectionState(bool connected)
{
    if (!m_photoshopConnectionLabel)
    {
        return;
    }

    if (connected)
    {
        m_photoshopConnectionLabel->setText(QStringLiteral("<span style=\"color:#34C759;\">"
                                                           "●"
                                                           "</span> "
                                                           "Photoshop conectado"));

        m_photoshopConnectionLabel->setToolTip(QStringLiteral("LinksFlow está conectado "
                                                              "con Adobe Photoshop."));
    }
    else
    {
        m_photoshopConnectionLabel->setText(QStringLiteral("<span style=\"color:#FF3B30;\">"
                                                           "●"
                                                           "</span> "
                                                           "Photoshop desconectado"));

        m_photoshopConnectionLabel->setToolTip(QStringLiteral("LinksFlow no está conectado "
                                                              "con Adobe Photoshop."));
    }
}
