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

    // Helper local: si algo falla después de haber creado la carpeta, eliminamos el backup incompleto.

    const auto failBackup = [&result, &backupDirectory](const QString &message) -> BackupResult {
        result.success = false;
        result.errorMessage = message;
        result.copiedFiles = 0;

        QDir directory(backupDirectory);

        if (directory.exists())
            directory.removeRecursively();

        result.backupDirectory.clear();

        return result;
    };

    //
    // Protección adicional contra duplicados.
    //

    QSet<QString> copiedSources;

    for (const ProcessingJob &job : jobs)
    {
        const QFileInfo sourceInfo(job.sourcePath);

        QString sourcePath = sourceInfo.canonicalFilePath();

        if (sourcePath.isEmpty())
        {
            sourcePath = sourceInfo.absoluteFilePath();
        }

        //
        // Un job sin ruta válida se considera error.
        // No conviene ignorarlo silenciosamente.
        //

        if (sourcePath.isEmpty())
        {
            return failBackup(tr("No se pudo determinar la ruta "
                                 "del archivo:\n%1")
                                  .arg(job.sourcePath));
        }

        if (copiedSources.contains(sourcePath))
        {
            continue;
        }

        copiedSources.insert(sourcePath);

        //
        // Verificar el origen.
        //

        if (!sourceInfo.exists() || !sourceInfo.isFile())
        {
            return failBackup(tr("No se encontró el archivo:\n%1").arg(job.sourcePath));
        }

        //
        // Determinar destino.
        //

        QString destinationPath = destinationForFile(sourcePath, projectDirectory, backupDirectory);

        if (destinationPath.isEmpty())
        {
            return failBackup(tr("No se pudo determinar dónde "
                                 "guardar la copia de:\n%1")
                                  .arg(sourcePath));
        }

        //
        // Crear carpetas intermedias.
        //

        const QFileInfo destinationInfo(destinationPath);

        QDir destinationDirectory;

        if (!destinationDirectory.mkpath(destinationInfo.absolutePath()))
        {
            return failBackup(tr("No se pudo crear la carpeta:\n%1").arg(destinationInfo.absolutePath()));
        }

        //
        // Evitar colisiones.
        //

        destinationPath = uniqueDestinationPath(destinationPath);

        //
        // Copiar.
        //

        if (!QFile::copy(sourcePath, destinationPath))
        {
            return failBackup(tr("No se pudo crear la copia "
                                 "de seguridad de:\n%1")
                                  .arg(sourcePath));
        }

        ++result.copiedFiles;
    }

    //
    // Un backup de cero archivos tampoco
    // debería considerarse válido.
    //

    if (result.copiedFiles == 0)
    {
        return failBackup(tr("No había archivos para incluir "
                             "en la copia de seguridad."));
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
