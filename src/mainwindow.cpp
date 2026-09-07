#include "mainwindow.h"
#include "services/adobeindesignbridge.h"
#include "services/indesignbridge.h"
#include "services/processingcontroller.h"
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

    m_processingController = new ProcessingController(this);

    statusBar()->addPermanentWidget(m_indesignConnectionLabel);

    connect(m_adobeTransport, &AdobeBridgeTransport::connectionChanged, this,
            &MainWindow::updateInDesignConnectionState);

    if (!m_adobeTransport->start(17321))
        qWarning() << "No se pudo iniciar Adobe Bridge";

    m_indesignBridge = new AdobeInDesignBridge(m_adobeTransport, this);

    m_configurationPage = new ConfigurationPage(m_pages);

    m_pages->addWidget(m_configurationPage);

    m_analysisPage = new AnalysisPage(m_pages);

    m_pages->addWidget(m_analysisPage);

    setCentralWidget(m_pages);

    connect(m_configurationPage, &ConfigurationPage::exitRequested, qApp, &QApplication::quit);
    connect(m_analysisPage, &AnalysisPage::backRequested, this,
            [this]() { m_pages->setCurrentWidget(m_configurationPage); });

    connect(m_analysisPage, &AnalysisPage::exitRequested, qApp, &QApplication::quit);

    connect(m_configurationPage, &ConfigurationPage::analyzeDocumentRequested, this,
            [this]() { m_indesignBridge->analyzeActiveDocument(); });

    connect(m_analysisPage, &AnalysisPage::processRequested, this, [this]() {
        const QList<LinkInfo> links = m_analysisPage->links();

        const OptimizationSettings settings = m_configurationPage->currentSettings();

        const QList<ProcessingJob> jobs = m_processingController->createJobs(links, settings);

        // Por ahora solo validaremos los jobs.
        QString message;

        message += tr("Se generaron %1 trabajos.\n\n").arg(jobs.size());

        for (const ProcessingJob &job : jobs)
        {
            message += QStringLiteral("• ");
            message += job.sourcePath;

            if (job.resizeRequired)
            {
                message += tr("\n  - Ajustar resolución a %1 ppi").arg(job.targetResolution);
            }

            if (job.colorModeConversionRequired)
            {
                message += tr("\n  - Convertir modo de color");
            }

            if (job.colorProfileConversionRequired)
            {
                message += tr("\n  - Convertir perfil: %1").arg(job.targetIccProfile);
            }

            if (job.removeHiddenLayers)
            {
                message += tr("\n  - Eliminar capas ocultas");
            }

            if (job.mergeVisibleLayers)
            {
                message += tr("\n  - Combinar capas visibles");
            }

            if (job.flattenImage)
            {
                message += tr("\n  - Acoplar imagen");
            }

            if (job.alphaChannels == AlphaChannelHandling::Remove)
            {
                message += tr("\n  - Eliminar canales alfa");
            }

            if (job.formatConversionRequired)
            {
                message += tr("\n  - Convertir formato");
            }

            message += QStringLiteral("\n\n");
        }

        QMessageBox::information(this, tr("Trabajos de procesamiento"), message);
    });

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
