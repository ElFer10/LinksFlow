#include "previewservice.h"

#include <QFileInfo>
#include <QImageReader>
#include <QMetaObject>
#include <QPointer>
#include <QThreadPool>

PreviewService::PreviewService(QObject *parent) : QObject(parent) {
  m_cache.setMaxCost(64 * 1024);
}
void PreviewService::requestPreview(const QString &filePath, int requestId,
                                    int maximumSize) {
  const QString cacheKey =
      filePath + QStringLiteral("|") + QString::number(maximumSize);

  // Primero comprobamos la caché.
  if (QImage *cachedImage = m_cache.object(cacheKey)) {
    emit previewReady(requestId, filePath, *cachedImage);

    return;
  }

  QPointer<PreviewService> self(this);

  QThreadPool::globalInstance()->start([self, filePath, requestId, maximumSize,
                                        cacheKey]() {
    if (!self) {
      return;
    }

    const QFileInfo fileInfo(filePath);

    if (!fileInfo.exists() || !fileInfo.isFile()) {
      QMetaObject::invokeMethod(
          self,
          [self, requestId, filePath]() {
            if (!self) {
              return;
            }

            emit self->previewFailed(requestId, filePath);
          },
          Qt::QueuedConnection);

      return;
    }

    QImageReader reader(filePath);
    reader.setAutoTransform(true);

    const QSize originalSize = reader.size();

    if (originalSize.isValid() && (originalSize.width() > maximumSize ||
                                   originalSize.height() > maximumSize)) {
      const QSize scaledSize =
          originalSize.scaled(maximumSize, maximumSize, Qt::KeepAspectRatio);

      reader.setScaledSize(scaledSize);
    }

    const QImage image = reader.read();

    if (!self) {
      return;
    }

    if (image.isNull()) {
      QMetaObject::invokeMethod(
          self,
          [self, requestId, filePath]() {
            if (!self) {
              return;
            }

            emit self->previewFailed(requestId, filePath);
          },
          Qt::QueuedConnection);

      return;
    }

    QMetaObject::invokeMethod(
        self,
        [self, requestId, filePath, cacheKey, image]() {
          if (!self) {
            return;
          }

          const qsizetype imageBytes = image.sizeInBytes();

          const int costKb = qMax(1, static_cast<int>(imageBytes / 1024));

          self->m_cache.insert(cacheKey, new QImage(image), costKb);

          emit self->previewReady(requestId, filePath, image);
        },
        Qt::QueuedConnection);
  });
}
