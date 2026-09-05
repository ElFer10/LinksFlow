#pragma once

#include <QList>
#include <QWidget>

#include "../../domain/linkinfo.h"

class QTableView;
class QLabel;
class QPushButton;
class QSortFilterProxyModel;

class LinksTableModel;
class PreviewService;

class AnalysisPage : public QWidget {
  Q_OBJECT

public:
  explicit AnalysisPage(QWidget *parent = nullptr);

  void setLinks(const QList<LinkInfo> &links);

signals:
  void backRequested();
  void processRequested();
  void exitRequested();

private:
  void setupUi();
  void setupConnections();
  void updateInspector(const QModelIndex &proxyIndex);
  void clearInspector();

  QString formatResolution(const Resolution2D &resolution) const;
  QString formatScale(const Scale2D &scale) const;
  QString formatFlip(const FlipState &flip) const;
  void updateProcessingState();
  QString formatState(LinkProcessState state) const;
  QString formatFileSize(qint64 bytes) const;
  void updatePreview(const LinkInfo &link);

private:
  QTableView *m_table = nullptr;

  QLabel *m_fileNameValue = nullptr;
  QLabel *m_pathValue = nullptr;
  QLabel *m_actualResolutionValue = nullptr;
  QLabel *m_effectiveResolutionValue = nullptr;
  QLabel *m_scaleValue = nullptr;
  QLabel *m_rotationValue = nullptr;
  QLabel *m_flipValue = nullptr;

  QPushButton *m_exitButton = nullptr;
  QPushButton *m_backButton = nullptr;
  QPushButton *m_processButton = nullptr;

  LinksTableModel *m_model = nullptr;
  QSortFilterProxyModel *m_proxyModel = nullptr;

  QLabel *m_statusValue = nullptr;
  QLabel *m_selectionSummary = nullptr;

  QLabel *m_previewLabel = nullptr;

  QLabel *m_typeValue = nullptr;
  QLabel *m_colorModeValue = nullptr;
  QLabel *m_iccProfileValue = nullptr;
  QLabel *m_fileSizeValue = nullptr;

  PreviewService *m_previewService = nullptr;

  int m_previewRequestId = 0;
};
