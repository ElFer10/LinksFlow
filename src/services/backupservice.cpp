#include "backupservice.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>

BackupService::BackupService(QObject *parent) : QObject(parent)
{
}

BackupResult BackupService::createBackup(const QList<ProcessingJob> &jobs, const QString &documentPath) const
{
    BackupResult result;

    // Necesitamos conocer dónde está guardado el documento de InDesign.

    if (documentPath.isEmpty())
    {
        result.errorMessage = tr("No se conoce la ubicación "
                                 "del documento de InDesign.");

        return result;
    }

    const QFileInfo documentInfo(documentPath);

    const QString projectDirectory = documentInfo.absolutePath();

    if (projectDirectory.isEmpty())
    {
        result.errorMessage = tr("No se pudo determinar la carpeta "
                                 "del documento de InDesign.");

        return result;
    }

    //
    // Crear Links_backup, Links_backup_2...
    //

    const QString backupDirectory = createBackupDirectory(projectDirectory);

    if (backupDirectory.isEmpty())
    {
        result.errorMessage = tr("No se pudo crear la carpeta de copia de seguridad.");

        return result;
    }

    result.backupDirectory = backupDirectory;

    // Protección adicional.
    //
    // ProcessingController ya debería haber agrupado los jobs por archivo
    // físico, pero BackupService no depende de eso.

    QSet<QString> copiedSources;

    for (const ProcessingJob &job : jobs)
    {
        const QFileInfo sourceInfo(job.sourcePath);

        QString sourcePath = sourceInfo.canonicalFilePath();

        if (sourcePath.isEmpty())
            sourcePath = sourceInfo.absoluteFilePath();

        if (sourcePath.isEmpty())
            continue;

        if (copiedSources.contains(sourcePath))
            continue;

        copiedSources.insert(sourcePath);

        //
        // Si un archivo que vamos a modificar no existe, abortamos Todo el backup.
        //

        if (!sourceInfo.exists() || !sourceInfo.isFile())
        {
            result.errorMessage = tr("No se encontró el archivo:\n%1").arg(job.sourcePath);

            return result;
        }

        QString destinationPath = destinationForFile(sourcePath, projectDirectory, backupDirectory);

        if (destinationPath.isEmpty())
        {
            result.errorMessage = tr("No se pudo determinar dónde guardar la copia de:\n%1").arg(sourcePath);

            return result;
        }

        //
        // Crear las carpetas intermedias.
        //

        const QFileInfo destinationInfo(destinationPath);

        QDir destinationDirectory;

        if (!destinationDirectory.mkpath(destinationInfo.absolutePath()))
        {
            result.errorMessage = tr("No se pudo crear la carpeta:\n%1").arg(destinationInfo.absolutePath());

            return result;
        }

        //
        // Protección contra cualquier colisión.
        //

        destinationPath = uniqueDestinationPath(destinationPath);

        if (!QFile::copy(sourcePath, destinationPath))
        {
            result.errorMessage = tr("No se pudo crear la copia de seguridad de:\n%1").arg(sourcePath);

            return result;
        }

        ++result.copiedFiles;
    }

    result.success = true;

    return result;
}

QString BackupService::createBackupDirectory(const QString &projectDirectory) const
{
    QDir projectDir(projectDirectory);

    //
    // Primera ejecución:
    //
    // Links_backup
    //

    QString directoryName = QStringLiteral("Links_backup");

    QString candidate = projectDir.filePath(directoryName);

    if (!QFileInfo::exists(candidate))
    {

        if (QDir().mkpath(candidate))
            return candidate;

        return {};
    }

    //
    // Siguientes ejecuciones:
    //
    // Links_backup_2
    // Links_backup_3
    // ...
    //

    int index = 2;

    while (true)
    {

        directoryName = QStringLiteral("Links_backup_%1").arg(index);

        candidate = projectDir.filePath(directoryName);

        if (!QFileInfo::exists(candidate))
        {
            if (QDir().mkpath(candidate))
                return candidate;

            return {};
        }

        ++index;
    }
}

QString BackupService::destinationForFile(const QString &sourcePath, const QString &projectDirectory,
                                          const QString &backupDirectory) const
{
    const QFileInfo sourceInfo(sourcePath);

    QDir projectDir(projectDirectory);

    // Intentar determinar si el archivo vive dentro de la carpeta del proyecto.

    const QString relativePath = projectDir.relativeFilePath(sourcePath);

    const bool isExternal = relativePath == QStringLiteral("..") || relativePath.startsWith(QStringLiteral("../"));

    if (!isExternal)
    {

        //
        // Ejemplo: Proyecto/Links/foto.tif
        // pasa a: Links_backup/Links/foto.tif
        //

        return QDir(backupDirectory).filePath(relativePath);
    }

    //
    // Archivo externo.
    //
    // Lo colocamos inicialmente en: Links_backup/External/
    //
    // uniqueDestinationPath() resolverá nombres repetidos.
    //

    return QDir(backupDirectory).filePath(QStringLiteral("External/%1").arg(sourceInfo.fileName()));
}

QString BackupService::uniqueDestinationPath(const QString &destinationPath) const
{
    if (!QFileInfo::exists(destinationPath))
    {
        return destinationPath;
    }

    const QFileInfo info(destinationPath);

    const QString directory = info.absolutePath();

    const QString baseName = info.completeBaseName();

    const QString suffix = info.completeSuffix();

    int index = 2;

    while (true)
    {

        QString fileName;

        if (suffix.isEmpty())
            fileName = QStringLiteral("%1_%2").arg(baseName).arg(index);
        else
            fileName = QStringLiteral("%1_%2.%3").arg(baseName).arg(index).arg(suffix);

        const QString candidate = QDir(directory).filePath(fileName);

        if (!QFileInfo::exists(candidate))
            return candidate;

        ++index;
    }
}
