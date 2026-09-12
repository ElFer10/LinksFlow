#include "processingcontroller.h"
#include "../domain/linkupdateresult.h"

#include "adobephotoshopbridge.h"
#include "indesignbridge.h"

#include <QStringList>

#include <QFileInfo>
#include <QHash>
#include <QTimer>
#include <QUuid>
#include <QtGlobal>
#include <limits>

namespace
{

QString normalizedFilePath(const QString &path)
{
    const QFileInfo fileInfo(path);

    const QString canonicalPath = fileInfo.canonicalFilePath();

    if (!canonicalPath.isEmpty())
        return canonicalPath;

    return fileInfo.absoluteFilePath();
}

bool imageFormatFromString(const QString &value, ImageFormat &format)
{
    const QString normalized = value.trimmed().toUpper();

    if (normalized == QStringLiteral("PSD"))
    {
        format = ImageFormat::PSD;
        return true;
    }

    if (normalized == QStringLiteral("TIFF") || normalized == QStringLiteral("TIF"))
    {
        format = ImageFormat::TIFF;
        return true;
    }

    if (normalized == QStringLiteral("JPEG") || normalized == QStringLiteral("JPG"))
    {
        format = ImageFormat::JPEG;
        return true;
    }

    if (normalized == QStringLiteral("PNG"))
    {
        format = ImageFormat::PNG;
        return true;
    }

    if (normalized == QStringLiteral("WEBP"))
    {
        format = ImageFormat::WebP;
        return true;
    }

    if (normalized == QStringLiteral("BMP"))
    {
        format = ImageFormat::BMP;
        return true;
    }

    return false;
}

const ConversionRule *findConversionRule(const ConversionSettings &settings, ImageFormat sourceFormat)
{
    for (const ConversionRule &rule : settings.rules)
    {
        if (rule.sourceFormat == sourceFormat)
            return &rule;
    }

    return nullptr;
}

} // namespace

ProcessingController::ProcessingController(AdobePhotoshopBridge *photoshopBridge, InDesignBridge *indesignBridge,
                                           QObject *parent)
    : QObject(parent), m_photoshopBridge(photoshopBridge), m_indesignBridge(indesignBridge)
{
    if (m_photoshopBridge)
    {
        connect(m_photoshopBridge, &AdobePhotoshopBridge::resolutionProcessed, this,
                &ProcessingController::handleResolutionProcessed);

        connect(m_photoshopBridge, &AdobePhotoshopBridge::resolutionProcessingFailed, this,
                &ProcessingController::handleResolutionProcessingFailed);
    }

    if (m_indesignBridge)
    {
        connect(m_indesignBridge, &InDesignBridge::linksUpdated, this, &ProcessingController::handleLinksUpdated);

        connect(m_indesignBridge, &InDesignBridge::linksUpdateFailed, this,
                &ProcessingController::handleLinksUpdateFailed);
    }
}

QList<ProcessingJob> ProcessingController::createJobs(const QList<LinkInfo> &links,
                                                      const OptimizationSettings &settings) const
{
    QList<ProcessingJob> jobs;

    // --------------------------------------------------------
    // 1. Determinar qué archivos físicos deben procesarse.
    //
    // Si al menos una colocación está seleccionada, procesamos ese archivo.
    // --------------------------------------------------------

    QHash<QString, bool> selectedSourcePaths;

    for (const LinkInfo &link : links)
    {

        if (!link.process)
            continue;

        if (link.state != LinkProcessState::Ready)
            continue;

        const QString sourceKey = normalizedFilePath(link.filePath);

        if (!sourceKey.isEmpty())
            selectedSourcePaths.insert(sourceKey, true);
    }

    // --------------------------------------------------------
    // 2. Crear un job por archivo físico.
    // --------------------------------------------------------

    QHash<QString, int> jobIndexBySourcePath;

    for (const LinkInfo &link : links)
    {

        // Una colocación no procesable no puede participar en el cálculo de la imagen.

        if (link.state != LinkProcessState::Ready)
            continue;

        const QString sourceKey = normalizedFilePath(link.filePath);

        if (sourceKey.isEmpty())
            continue;

        // Nadie seleccionó este archivo.

        if (!selectedSourcePaths.contains(sourceKey))
            continue;

        // El formato debe ser soportado.

        ImageFormat sourceFormat;

        if (!imageFormatFromString(link.fileType, sourceFormat))
            continue;

        int jobIndex = -1;

        // ¿Ya existe el job de este archivo?

        const auto existingJob = jobIndexBySourcePath.constFind(sourceKey);

        if (existingJob != jobIndexBySourcePath.constEnd())
        {
            jobIndex = existingJob.value();
        }
        else
        {

            //
            // Crear el job físico.
            //

            ProcessingJob job;

            job.id = QUuid::createUuid().toString(QUuid::WithoutBraces);

            job.sourcePath = link.filePath;

            // Por ahora seguimos trabajando sobre el original.
            //
            // BackupService se ocupará después de protegerlo antes de Photoshop.

            job.outputPath = link.filePath;

            job.sourceFormat = sourceFormat;

            job.targetFormat = sourceFormat;

            // Resolución

            job.resolutionUnit = settings.resolution.unit;

            job.optimizationMethod = settings.resolution.optimizationMethod;

            job.cropToInDesignFrame = settings.resolution.cropToInDesignFrame;

            job.safetyArea = settings.resolution.safetyArea;

            job.safetyAreaUnit = settings.resolution.safetyAreaUnit;

            // Por ahora seguimos usando resolución de tono continuo.
            //
            // Más adelante añadiremos la distinción bitmap/1-bit.

            job.targetResolution = settings.resolution.colorResolution;

            //
            // Edición
            //

            const ImageEditingSettings &editing = settings.imageEditing;

            job.colorModeConversionRequired = editing.changeColorMode;

            job.sourceColorMode = editing.sourceColorMode;

            job.destinationColorMode = editing.destinationColorMode;

            job.colorProfileConversionRequired = editing.changeColorProfile;

            job.targetIccProfile = editing.iccProfile;

            job.removeHiddenLayers = editing.removeHiddenLayers;

            job.mergeVisibleLayers = editing.mergeVisibleLayers;

            job.flattenImage = editing.flattenImage;

            job.alphaChannels = editing.alphaChannels;

            //
            // Conversión de formato
            //

            if (settings.conversion.enabled)
            {
                const ConversionRule *rule = findConversionRule(settings.conversion, sourceFormat);

                if (rule != nullptr && rule->enabled)
                {
                    job.targetFormat = rule->destinationFormat;

                    job.formatConversionRequired = (rule->destinationFormat != sourceFormat);

                    job.formatOptions = rule->options;
                }
            }

            //
            // Estado inicial
            //

            job.state = ProcessingJobState::Pending;

            job.statusMessage.clear();

            jobs.append(job);

            jobIndex = jobs.size() - 1;

            jobIndexBySourcePath.insert(sourceKey, jobIndex);
        }

        // ----------------------------------------------------
        // 3. Añadir esta colocación al job.
        // ----------------------------------------------------

        ProcessingJob &job = jobs[jobIndex];

        ImageUsage usage;

        usage.indesignLinkId = link.indesignLinkId;

        usage.indesignPageItemId = link.indesignPageItemId;

        usage.page = link.page;

        usage.actualResolution = link.actualResolution;

        usage.effectiveResolution = link.effectiveResolution;

        usage.scale = link.scale;

        usage.rotation = link.rotation;

        usage.flip = link.flip;

        job.usages.append(usage);
    }

    // --------------------------------------------------------
    // 4. Determinar la colocación más exigente.
    // --------------------------------------------------------

    for (ProcessingJob &job : jobs)
    {

        double minimumX = std::numeric_limits<double>::max();

        double minimumY = std::numeric_limits<double>::max();

        for (const ImageUsage &usage : job.usages)
        {
            if (usage.effectiveResolution.x > 0.0)
                minimumX = qMin(minimumX, usage.effectiveResolution.x);

            if (usage.effectiveResolution.y > 0.0)
                minimumY = qMin(minimumY, usage.effectiveResolution.y);
        }

        if (minimumX == std::numeric_limits<double>::max())
            minimumX = 0.0;

        if (minimumY == std::numeric_limits<double>::max())
            minimumY = 0.0;

        job.minimumEffectiveResolutionX = minimumX;

        job.minimumEffectiveResolutionY = minimumY;

        // Solo es seguro reducir si TODAS las colocaciones superan la resolución objetivo.
        //
        // Si una colocación ya está en 250 ppi y nuestro objetivo son 300 ppi,
        // no podemos reducir físicamente el archivo.

        job.resizeRequired = (minimumX > job.targetResolution && minimumY > job.targetResolution);
    }

    return jobs;
}

void ProcessingController::processJobs(const QList<ProcessingJob> &jobs)
{
    if (m_processing)
    {
        emit processingFailed(tr("Ya hay un procesamiento en curso."));

        return;
    }

    if (jobs.isEmpty())
    {
        emit processingCompleted();
        return;
    }

    m_jobs = jobs;

    m_currentJobIndex = -1;

    m_cancelRequested = false;
    m_processing = true;

    m_currentOriginalSizeBytes = 0;

    emit processingStarted(m_jobs.size());

    processNextJob();
}

void ProcessingController::cancelProcessing()
{
    if (!m_processing)
        return;

    // Por ahora no podemos interrumpir a Photoshop en mitad de un executeAsModal.
    //
    // El job actualmente activo terminará y los restantes serán omitidos.

    m_cancelRequested = true;
}

void ProcessingController::processNextJob()
{
    // ========================================================
    // CANCELACIÓN
    // ========================================================

    if (m_cancelRequested)
    {
        for (int index = m_currentJobIndex + 1; index < m_jobs.size(); ++index)
        {
            ProcessingJob &job = m_jobs[index];

            job.state = ProcessingJobState::Skipped;

            job.statusMessage = tr("Cancelado por el usuario");

            ProcessingResult result;

            result.jobId = job.id;

            result.state = ProcessingJobState::Skipped;

            result.sourcePath = job.sourcePath;

            result.outputPath = job.outputPath;

            result.message = job.statusMessage;

            const QFileInfo fileInfo(job.sourcePath);

            if (fileInfo.exists() && fileInfo.isFile())
            {
                result.originalSizeBytes = fileInfo.size();

                result.processedSizeBytes = fileInfo.size();
            }

            emit jobCompleted(result);
        }

        m_processing = false;

        emit processingCompleted();

        return;
    }

    // ========================================================
    // SIGUIENTE JOB
    // ========================================================

    ++m_currentJobIndex;

    if (m_currentJobIndex >= m_jobs.size())
    {
        m_processing = false;

        emit processingCompleted();

        return;
    }

    ProcessingJob &job = m_jobs[m_currentJobIndex];

    //
    // ========================================================
    // OPERACIONES IMPLEMENTADAS
    // ========================================================
    //
    // En este momento Photoshop solamente implementa reducción de resolución.
    //

    if (!job.resizeRequired)
    {
        job.state = ProcessingJobState::Skipped;

        job.statusMessage = tr("No requiere reducción de resolución.");

        ProcessingResult result;

        result.jobId = job.id;

        result.state = ProcessingJobState::Skipped;

        result.sourcePath = job.sourcePath;

        result.outputPath = job.outputPath;

        result.message = job.statusMessage;

        const QFileInfo fileInfo(job.sourcePath);

        if (fileInfo.exists() && fileInfo.isFile())
        {
            result.originalSizeBytes = fileInfo.size();

            result.processedSizeBytes = fileInfo.size();
        }

        emit jobCompleted(result);

        // Evitamos encadenar llamadas recursivas si hay muchos jobs omitidos.

        QTimer::singleShot(0, this, &ProcessingController::processNextJob);

        return;
    }

    // ========================================================
    // PHOTOSHOP DISPONIBLE
    // ========================================================

    if (!m_photoshopBridge || !m_photoshopBridge->isConnected())
    {
        finishCurrentJob(ProcessingJobState::Failed, tr("Adobe Photoshop no está conectado."));

        return;
    }

    // ========================================================
    // FACTOR DE REDUCCIÓN
    // ========================================================

    const double scaleFactor = scaleFactorForJob(job);

    if (scaleFactor <= 0.0 || scaleFactor >= 1.0)
    {
        finishCurrentJob(ProcessingJobState::Failed, tr("No se pudo calcular un factor de reducción válido."));

        return;
    }

    //
    // ========================================================
    // COMENZAR JOB REAL
    // ========================================================
    //

    const QFileInfo fileInfo(job.sourcePath);

    m_currentOriginalSizeBytes = (fileInfo.exists() && fileInfo.isFile()) ? fileInfo.size() : 0;

    job.state = ProcessingJobState::Processing;

    job.statusMessage = tr("Procesando en Photoshop");

    emit jobStarted(job);

    m_photoshopBridge->processResolution(job.sourcePath, scaleFactor);
}

double ProcessingController::scaleFactorForJob(const ProcessingJob &job) const
{
    if (job.targetResolution <= 0.0 || job.minimumEffectiveResolutionX <= 0.0 || job.minimumEffectiveResolutionY <= 0.0)
        return 0.0;

    const double factorX = job.targetResolution / job.minimumEffectiveResolutionX;

    const double factorY = job.targetResolution / job.minimumEffectiveResolutionY;

    // Elegimos el factor mayor para garantizar que ninguna dimensión de
    // ninguna colocación quede por debajo del target.

    return qMax(factorX, factorY);
}

void ProcessingController::handleResolutionProcessed(const PhotoshopProcessResult &result)
{
    Q_UNUSED(result);

    if (!m_processing || m_currentJobIndex < 0 || m_currentJobIndex >= m_jobs.size())
    {
        return;
    }

    ProcessingJob &job = m_jobs[m_currentJobIndex];

    // Photoshop terminó. Ahora InDesign debe releer todas las colocaciones
    // correspondientes al archivo.

    if (!m_indesignBridge)
    {
        finishCurrentJob(ProcessingJobState::Failed, tr("El bridge de InDesign no está disponible."));

        return;
    }

    QList<qint64> linkIds;

    for (const ImageUsage &usage : job.usages)
    {
        if (usage.indesignLinkId > 0 && !linkIds.contains(usage.indesignLinkId))
        {
            linkIds.append(usage.indesignLinkId);
        }
    }

    if (linkIds.isEmpty())
    {
        finishCurrentJob(ProcessingJobState::Failed, tr("No se encontraron vínculos de InDesign para actualizar."));

        return;
    }

    job.statusMessage = tr("Actualizando vínculos en InDesign");

    m_indesignBridge->updateLinks(linkIds);
}

void ProcessingController::handleResolutionProcessingFailed(const QString &message)
{
    if (!m_processing || m_currentJobIndex < 0 || m_currentJobIndex >= m_jobs.size())
    {
        return;
    }

    finishCurrentJob(ProcessingJobState::Failed, message);
}

void ProcessingController::finishCurrentJob(ProcessingJobState state, const QString &message)
{
    if (m_currentJobIndex < 0 || m_currentJobIndex >= m_jobs.size())
    {
        return;
    }

    ProcessingJob &job = m_jobs[m_currentJobIndex];

    job.state = state;

    job.statusMessage = message;

    ProcessingResult result;

    result.jobId = job.id;

    result.state = state;

    result.sourcePath = job.sourcePath;

    result.outputPath = job.outputPath;

    result.originalSizeBytes = m_currentOriginalSizeBytes;

    const QFileInfo fileInfo(job.outputPath);

    if (fileInfo.exists() && fileInfo.isFile())
    {
        result.processedSizeBytes = fileInfo.size();

        if (result.originalSizeBytes == 0)
        {
            result.originalSizeBytes = fileInfo.size();
        }
    }

    result.message = message;

    emit jobCompleted(result);

    m_currentOriginalSizeBytes = 0;

    QTimer::singleShot(0, this, &ProcessingController::processNextJob);
}

void ProcessingController::handleLinksUpdated(const LinksUpdateResult &result)
{
    if (!m_processing || m_currentJobIndex < 0 || m_currentJobIndex >= m_jobs.size())
        return;

    const ProcessingJob &job = m_jobs[m_currentJobIndex];

    qDebug() << "Links actualizados:" << result.updated << "de" << result.requested;

    if (result.updated <= 0 || result.links.isEmpty())
    {
        finishCurrentJob(ProcessingJobState::Failed, tr("InDesign no pudo actualizar ninguna colocación del archivo."));
        return;
    }

    // ========================================================
    // VALIDACIÓN POSTPROCESAMIENTO
    // ========================================================

    constexpr double tolerancePpi = 300;

    bool validationOk = true;

    QStringList validationErrors;

    for (const UpdatedLinkInfo &link : result.links)
    {
        if (!link.success)
        {
            validationOk = false;

            validationErrors.append(tr("Link %1 no pudo actualizarse.").arg(link.linkId));

            continue;
        }

        if (link.statusAfter != QStringLiteral("NORMAL"))
        {
            validationOk = false;

            validationErrors.append(tr("Link %1 quedó en estado %2.").arg(link.linkId).arg(link.statusAfter));

            continue;
        }

        // Solo validamos contra target si este job realmente hizo resize.

        if (job.resizeRequired)
        {
            const double minimumAccepted = job.targetResolution - tolerancePpi;

            if (link.effectiveResolutionX < minimumAccepted || link.effectiveResolutionY < minimumAccepted)
            {
                validationOk = false;

                validationErrors.append(tr("Link %1 quedó en "
                                           "%2 × %3 ppi efectivos; "
                                           "objetivo: %4 ppi.")
                                            .arg(link.linkId)
                                            .arg(link.effectiveResolutionX, 0, 'f', 1)
                                            .arg(link.effectiveResolutionY, 0, 'f', 1)
                                            .arg(job.targetResolution, 0, 'f', 1));
            }
        }
    }

    if (!validationOk)
    {
        finishCurrentJob(ProcessingJobState::Failed, tr("La validación posterior en "
                                                        "InDesign falló:\n%1")
                                                         .arg(validationErrors.join(QStringLiteral("\n"))));

        return;
    }

    completeCurrentJob();
}

void ProcessingController::handleLinksUpdateFailed(const QString &message)
{
    if (!m_processing || m_currentJobIndex < 0 || m_currentJobIndex >= m_jobs.size())
        return;

    finishCurrentJob(ProcessingJobState::Failed, message);
}

void ProcessingController::completeCurrentJob()
{
    if (m_currentJobIndex < 0 || m_currentJobIndex >= m_jobs.size())
        return;

    ProcessingJob &job = m_jobs[m_currentJobIndex];

    job.state = ProcessingJobState::Completed;

    job.statusMessage = tr("Optimizado y actualizado en InDesign");

    ProcessingResult result;

    result.jobId = job.id;

    result.state = ProcessingJobState::Completed;

    result.sourcePath = job.sourcePath;

    result.outputPath = job.outputPath;

    result.originalSizeBytes = m_currentOriginalSizeBytes;

    const QFileInfo processedInfo(job.outputPath);

    if (processedInfo.exists() && processedInfo.isFile())
        result.processedSizeBytes = processedInfo.size();

    result.message = job.statusMessage;

    emit jobCompleted(result);

    m_currentOriginalSizeBytes = 0;

    QTimer::singleShot(0, this, &ProcessingController::processNextJob);
}
