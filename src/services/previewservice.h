#pragma once

#include <QCache>
#include <QImage>
#include <QObject>
#include <QString>

class PreviewService : public QObject {
  Q_OBJECT

public:
  explicit PreviewService(QObject *parent = nullptr);

  void requestPreview(const QString &filePath, int requestId,
                      int maximumSize = 600);

signals:
  void previewReady(int requestId, const QString &filePath,
                    const QImage &image);

  void previewFailed(int requestId, const QString &filePath);

private:
  QCache<QString, QImage> m_cache;
};
