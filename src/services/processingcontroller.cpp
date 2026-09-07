#include "processingcontroller.h"

#include <QFileInfo>
#include <QTimer>
#include <QUuid>

namespace
{

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

    for (const LinkInfo &link : links)
    {

        // Solo procesamos links marcados por el usuario.
        if (!link.process)
        {
            continue;
        }

        // Solo procesamos links técnicamente válidos.
        if (link.state != LinkProcessState::Ready)
        {
            continue;
        }

        ImageFormat sourceFormat;

        if (!imageFormatFromString(link.fileType, sourceFormat))
        {
            continue;
        }

        ProcessingJob job;

        //
        // Identificación
        //

        job.id = QUuid::createUuid().toString(QUuid::WithoutBraces);

        //
        // Archivo
        //

        job.sourcePath = link.filePath;

        // Por ahora trabajamos sobre el mismo path.
        // Más adelante podremos cambiar esta lógica
        // cuando implementemos backups y conversiones.
        job.outputPath = link.filePath;

        job.sourceFormat = sourceFormat;

        job.targetFormat = sourceFormat;

        //
        // Referencia en InDesign
        //

        job.indesignLinkId = link.indesignLinkId;

        job.indesignPageItemId = link.indesignPageItemId;

        //
        // Resolución
        //

        job.effectiveResolutionX = link.effectiveResolution.x;

        job.effectiveResolutionY = link.effectiveResolution.y;

        job.resolutionUnit = settings.resolution.unit;

        job.optimizationMethod = settings.resolution.optimizationMethod;

        job.cropToInDesignFrame = settings.resolution.cropToInDesignFrame;

        job.safetyArea = settings.resolution.safetyArea;

        job.safetyAreaUnit = settings.resolution.safetyAreaUnit;

        // Por ahora usamos la resolución de color
        // como objetivo general.
        //
        // Más adelante distinguiremos correctamente
        // imágenes continuas de imágenes monocromas/1-bit.
        job.targetResolution = settings.resolution.colorResolution;

        job.resizeRequired =
            job.effectiveResolutionX > job.targetResolution || job.effectiveResolutionY > job.targetResolution;

        //
        // Edición de imagen
        //

        const ImageEditingSettings &editing = settings.imageEditing;

        //
        // Conversión de modo de color
        //

        job.colorModeConversionRequired = editing.changeColorMode;

        job.sourceColorMode = editing.sourceColorMode;

        job.destinationColorMode = editing.destinationColorMode;

        //
        // Conversión de perfil ICC
        //

        job.colorProfileConversionRequired = editing.changeColorProfile;

        job.targetIccProfile = editing.iccProfile;

        //
        // Capas
        //

        job.removeHiddenLayers = editing.removeHiddenLayers;

        job.mergeVisibleLayers = editing.mergeVisibleLayers;

        job.flattenImage = editing.flattenImage;

        //
        // Canales alfa
        //

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

                job.formatConversionRequired = rule->destinationFormat != sourceFormat;

                job.formatOptions = rule->options;
            }
        }
        //
        // Estado inicial
        //

        job.state = ProcessingJobState::Pending;

        job.statusMessage.clear();

        jobs.append(job);
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

        // Marcamos como omitidos los trabajos
        // que todavía no comenzaron.

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

    //
    // Simulación temporal.
    //
    // Más adelante este bloque será reemplazado
    // por PhotoshopBridge.
    //

    QTimer::singleShot(800, this, [this]() {
        if (m_currentJobIndex < 0 || m_currentJobIndex >= m_jobs.size())
        {
            return;
        }

        ProcessingJob &job = m_jobs[m_currentJobIndex];

        ProcessingResult result;

        result.jobId = job.id;

        result.sourcePath = job.sourcePath;

        result.outputPath = job.outputPath;

        const QFileInfo fileInfo(job.sourcePath);

        if (fileInfo.exists() && fileInfo.isFile())
        {
            result.originalSizeBytes = fileInfo.size();

            //
            // Simulamos una reducción del 30 %.
            //
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
