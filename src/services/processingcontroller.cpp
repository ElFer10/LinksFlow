#include "processingcontroller.h"

#include <QFileInfo>
#include <QHash>
#include <QTimer>
#include <QUuid>
#include <limits>

namespace
{

QString normalizedFilePath(const QString &path)
{
    const QFileInfo fileInfo(path);

    const QString canonicalPath = fileInfo.canonicalFilePath();

    if (!canonicalPath.isEmpty())
    {
        return canonicalPath;
    }

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
        {
            return &rule;
        }
    }

    return nullptr;
}

} // namespace

ProcessingController::ProcessingController(QObject *parent) : QObject(parent)
{
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
    if (jobs.isEmpty())
    {
        emit processingCompleted();
        return;
    }

    m_jobs = jobs;
    m_currentJobIndex = -1;
    m_cancelRequested = false;

    emit processingStarted(m_jobs.size());

    processNextJob();
}

void ProcessingController::cancelProcessing()
{
    m_cancelRequested = true;
}

void ProcessingController::processNextJob()
{
    if (m_cancelRequested)
    {

        // Marcamos como omitidos los trabajos que todavía no comenzaron.

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

        emit processingCompleted();
        return;
    }

    ++m_currentJobIndex;

    if (m_currentJobIndex >= m_jobs.size())
    {
        emit processingCompleted();
        return;
    }

    ProcessingJob &job = m_jobs[m_currentJobIndex];

    job.state = ProcessingJobState::Processing;

    emit jobStarted(job);

    // Simulación temporal.
    //
    // Más adelante este bloque será reemplazado por PhotoshopBridge.

    QTimer::singleShot(800, this, [this]() {
        if (m_currentJobIndex < 0 || m_currentJobIndex >= m_jobs.size())
            return;

        ProcessingJob &job = m_jobs[m_currentJobIndex];

        ProcessingResult result;

        result.jobId = job.id;

        result.sourcePath = job.sourcePath;

        result.outputPath = job.outputPath;

        const QFileInfo fileInfo(job.sourcePath);

        if (fileInfo.exists() && fileInfo.isFile())
        {
            result.originalSizeBytes = fileInfo.size();

            // Simulamos una reducción del 30 %.
            result.processedSizeBytes = static_cast<qint64>(fileInfo.size() * 0.70);
        }

        if (m_cancelRequested)
        {

            job.state = ProcessingJobState::Skipped;

            job.statusMessage = tr("Cancelado por el usuario");

            result.state = ProcessingJobState::Skipped;

            result.message = job.statusMessage;
        }
        else
        {

            job.state = ProcessingJobState::Completed;

            job.statusMessage = tr("Procesamiento simulado");

            result.state = ProcessingJobState::Completed;

            result.message = job.statusMessage;
        }

        emit jobCompleted(result);

        processNextJob();
    });
}
