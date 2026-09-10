#pragma once

#include "../domain/processingjob.h"

#include <QList>
#include <QObject>
#include <QString>

struct BackupResult
{
    bool success = false;

    QString backupDirectory;
    QString errorMessage;

    int copiedFiles = 0;
};

class BackupService : public QObject
{
    Q_OBJECT

  public:
    explicit BackupService(QObject *parent = nullptr);

    BackupResult createBackup(const QList<ProcessingJob> &jobs, const QString &documentPath) const;

  private:
    QString createBackupDirectory(const QString &projectDirectory) const;

    QString destinationForFile(const QString &sourcePath, const QString &projectDirectory,
                               const QString &backupDirectory) const;

    QString uniqueDestinationPath(const QString &destinationPath) const;
};
