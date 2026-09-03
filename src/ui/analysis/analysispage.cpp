#include "analysispage.h"

#include "../../models/linkstablemodel.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QTableView>
#include <QVBoxLayout>
#include <QtMath>

AnalysisPage::AnalysisPage(QWidget *parent) : QWidget(parent) {
  setupUi();
  setupConnections();
  clearInspector();
}

void AnalysisPage::setupUi() {
  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(16, 16, 16, 16);
  mainLayout->setSpacing(12);

  auto *title = new QLabel(tr("Enlaces del documento"), this);

  QFont titleFont = title->font();
  titleFont.setBold(true);
  titleFont.setPointSizeF(titleFont.pointSizeF() + 2.0);
  title->setFont(titleFont);

  mainLayout->addWidget(title);

  auto *description =
      new QLabel(tr("Seleccione los enlaces que desea procesar."), this);

  mainLayout->addWidget(description);

  auto *splitter = new QSplitter(Qt::Vertical, this);

  // Tabla
  m_table = new QTableView(splitter);

  m_model = new LinksTableModel(this);

  m_proxyModel = new QSortFilterProxyModel(this);
  m_proxyModel->setSourceModel(m_model);
  m_proxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);

  m_table->setModel(m_proxyModel);

  m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

  m_table->setSelectionMode(QAbstractItemView::SingleSelection);

  m_table->setAlternatingRowColors(true);
  m_table->setSortingEnabled(true);
  m_table->setEditTriggers(QAbstractItemView::SelectedClicked |
                           QAbstractItemView::DoubleClicked);

  m_table->verticalHeader()->setVisible(false);
  m_table->verticalHeader()->setDefaultSectionSize(28);

  auto *header = m_table->horizontalHeader();

  header->setSectionResizeMode(LinksTableModel::ProcessColumn,
                               QHeaderView::ResizeToContents);

  header->setSectionResizeMode(LinksTableModel::PageColumn,
                               QHeaderView::ResizeToContents);

  header->setSectionResizeMode(LinksTableModel::NameColumn,
                               QHeaderView::Stretch);

  header->setSectionResizeMode(LinksTableModel::TypeColumn,
                               QHeaderView::ResizeToContents);

  header->setSectionResizeMode(LinksTableModel::ColorColumn,
                               QHeaderView::ResizeToContents);

  header->setSectionResizeMode(LinksTableModel::SizeColumn,
                               QHeaderView::ResizeToContents);

  header->setSectionResizeMode(LinksTableModel::EffectiveResolutionColumn,
                               QHeaderView::ResizeToContents);

  header->setSectionResizeMode(LinksTableModel::IccProfileColumn,
                               QHeaderView::ResizeToContents);

  splitter->addWidget(m_table);

  // Inspector
  auto *inspector = new QWidget(splitter);
  // auto *inspectorLayout = new QFormLayout(inspector);
  //
  // inspectorLayout->setContentsMargins(8, 8, 8, 8);
  auto *inspectorLayout = new QHBoxLayout(inspector);

  m_previewLabel = new QLabel(inspector);
  m_previewLabel->setFixedSize(180, 180);
  m_previewLabel->setAlignment(Qt::AlignCenter);
  m_previewLabel->setFrameShape(QFrame::StyledPanel);
  m_previewLabel->setText(tr("Sin previsualización"));

  inspectorLayout->addWidget(m_previewLabel, 0, Qt::AlignTop);

  auto *detailsWidget = new QWidget(inspector);
  auto *detailsLayout = new QFormLayout(detailsWidget);

  detailsLayout->setContentsMargins(0, 0, 0, 0);

  m_typeValue = new QLabel(detailsWidget);
  m_colorModeValue = new QLabel(detailsWidget);
  m_iccProfileValue = new QLabel(detailsWidget);
  m_fileSizeValue = new QLabel(detailsWidget);
  m_fileNameValue = new QLabel(inspector);
  m_pathValue = new QLabel(inspector);
  m_actualResolutionValue = new QLabel(inspector);
  m_effectiveResolutionValue = new QLabel(inspector);
  m_scaleValue = new QLabel(inspector);
  m_rotationValue = new QLabel(inspector);
  m_flipValue = new QLabel(inspector);
  m_statusValue = new QLabel(inspector);

  m_pathValue->setWordWrap(true);
  m_pathValue->setTextInteractionFlags(Qt::TextSelectableByMouse);

  detailsLayout->addRow(tr("Archivo:"), m_fileNameValue);
  detailsLayout->addRow(tr("Estado:"), m_statusValue);
  detailsLayout->addRow(tr("Ubicación:"), m_pathValue);
  detailsLayout->addRow(tr("Resolución real:"), m_actualResolutionValue);
  detailsLayout->addRow(tr("Resolución efectiva:"), m_effectiveResolutionValue);
  detailsLayout->addRow(tr("Escala:"), m_scaleValue);
  detailsLayout->addRow(tr("Rotación:"), m_rotationValue);
  detailsLayout->addRow(tr("Volteado:"), m_flipValue);
  detailsLayout->addRow(tr("Tipo:"), m_typeValue);
  detailsLayout->addRow(tr("Color:"), m_colorModeValue);
  detailsLayout->addRow(tr("Perfil ICC:"), m_iccProfileValue);
  detailsLayout->addRow(tr("Tamaño:"), m_fileSizeValue);

  inspectorLayout->addWidget(detailsWidget, 1);

  splitter->addWidget(inspector);

  splitter->setStretchFactor(0, 3);
  splitter->setStretchFactor(1, 1);

  mainLayout->addWidget(splitter, 1);

  // Barra inferior
  auto *buttonLayout = new QHBoxLayout();
  m_selectionSummary = new QLabel(this);

  m_exitButton = new QPushButton(tr("Salir"), this);

  m_backButton = new QPushButton(tr("Cancelar"), this);

  m_processButton = new QPushButton(tr("Procesar"), this);

  m_processButton->setDefault(true);

  buttonLayout->addWidget(m_exitButton);
  buttonLayout->addWidget(m_selectionSummary);
  buttonLayout->addStretch();
  buttonLayout->addWidget(m_backButton);
  buttonLayout->addWidget(m_processButton);

  mainLayout->addLayout(buttonLayout);
}

void AnalysisPage::setupConnections() {
  connect(m_exitButton, &QPushButton::clicked, this,
          &AnalysisPage::exitRequested);

  connect(m_backButton, &QPushButton::clicked, this,
          &AnalysisPage::backRequested);

  connect(m_processButton, &QPushButton::clicked, this,
          &AnalysisPage::processRequested);

  connect(m_table->selectionModel(), &QItemSelectionModel::currentRowChanged,
          this, [this](const QModelIndex &current, const QModelIndex &) {
            if (!current.isValid()) {
              clearInspector();
              return;
            }

            updateInspector(current);
          });
  connect(m_model, &LinksTableModel::processSelectionChanged, this,
          &AnalysisPage::updateProcessingState);
}

void AnalysisPage::setLinks(const QList<LinkInfo> &links) {
  m_model->setLinks(links);

  if (m_proxyModel->rowCount() > 0) {
    const QModelIndex first = m_proxyModel->index(0, 0);

    m_table->setCurrentIndex(first);
    m_table->selectRow(0);

    updateInspector(first);
  } else {
    clearInspector();
  }

  updateProcessingState();
}

void AnalysisPage::updateInspector(const QModelIndex &proxyIndex) {
  if (!proxyIndex.isValid()) {
    clearInspector();
    return;
  }

  const QModelIndex sourceIndex = m_proxyModel->mapToSource(proxyIndex);

  const LinkInfo link = m_model->linkAt(sourceIndex.row());

  m_fileNameValue->setText(link.fileName);

  m_pathValue->setText(link.filePath);

  m_actualResolutionValue->setText(formatResolution(link.actualResolution));

  m_effectiveResolutionValue->setText(
      formatResolution(link.effectiveResolution));

  m_scaleValue->setText(formatScale(link.scale));

  m_rotationValue->setText(tr("%1°").arg(link.rotation, 0, 'f', 1));

  m_flipValue->setText(formatFlip(link.flip));

  QString statusText = formatState(link.state);

  if (!link.statusMessage.isEmpty()) {
    statusText += QStringLiteral(" — ") + link.statusMessage;
  }

  m_statusValue->setText(statusText);

  m_typeValue->setText(link.fileType.isEmpty() ? QStringLiteral("—")
                                               : link.fileType);

  m_colorModeValue->setText(link.colorMode.isEmpty() ? QStringLiteral("—")
                                                     : link.colorMode);

  m_iccProfileValue->setText(link.iccProfile.isEmpty() ? QStringLiteral("—")
                                                       : link.iccProfile);

  if (link.state == LinkProcessState::Missing ||
      link.state == LinkProcessState::Error) {
    m_fileSizeValue->setText(QStringLiteral("—"));
  } else {
    m_fileSizeValue->setText(formatFileSize(link.fileSizeBytes));
  }

  updatePreview(link);
}

void AnalysisPage::clearInspector() {
  m_fileNameValue->setText(QStringLiteral("—"));
  m_pathValue->setText(QStringLiteral("—"));
  m_actualResolutionValue->setText(QStringLiteral("—"));
  m_effectiveResolutionValue->setText(QStringLiteral("—"));
  m_scaleValue->setText(QStringLiteral("—"));
  m_rotationValue->setText(QStringLiteral("—"));
  m_flipValue->setText(QStringLiteral("—"));
  m_statusValue->setText(QStringLiteral("—"));
  m_previewLabel->setPixmap({});
  m_previewLabel->setText(tr("Sin previsualización"));
  m_typeValue->setText(QStringLiteral("—"));
  m_colorModeValue->setText(QStringLiteral("—"));
  m_iccProfileValue->setText(QStringLiteral("—"));
  m_fileSizeValue->setText(QStringLiteral("—"));
}

QString AnalysisPage::formatResolution(const Resolution2D &resolution) const {
  constexpr double tolerance = 0.5;

  if (qAbs(resolution.x - resolution.y) < tolerance) {
    return tr("%1 ppi").arg(qRound(resolution.x));
  }

  return tr("%1 × %2 ppi").arg(qRound(resolution.x)).arg(qRound(resolution.y));
}

QString AnalysisPage::formatScale(const Scale2D &scale) const {
  constexpr double tolerance = 0.01;

  if (qAbs(scale.horizontal - scale.vertical) < tolerance) {
    return tr("%1 %").arg(scale.horizontal, 0, 'f', 2);
  }

  return tr("%1 % × %2 %")
      .arg(scale.horizontal, 0, 'f', 2)
      .arg(scale.vertical, 0, 'f', 2);
}

QString AnalysisPage::formatFlip(const FlipState &flip) const {
  if (flip.horizontal && flip.vertical) {
    return tr("Horizontal y vertical");
  }

  if (flip.horizontal) {
    return tr("Horizontal");
  }

  if (flip.vertical) {
    return tr("Vertical");
  }

  return tr("No");
}

QString AnalysisPage::formatState(LinkProcessState state) const {
  switch (state) {
  case LinkProcessState::Ready:
    return tr("Listo");

  case LinkProcessState::Excluded:
    return tr("Excluido");

  case LinkProcessState::Missing:
    return tr("Archivo no encontrado");

  case LinkProcessState::Unsupported:
    return tr("No compatible");

  case LinkProcessState::Error:
    return tr("Error");
  }

  return {};
}

void AnalysisPage::updateProcessingState() {
  const int selected = m_model->selectedForProcessingCount();

  const int processable = m_model->processableCount();

  m_selectionSummary->setText(
      tr("%1 de %2 enlaces seleccionados").arg(selected).arg(processable));

  m_processButton->setEnabled(selected > 0);
}

QString AnalysisPage::formatFileSize(qint64 bytes) const {
  if (bytes < 1024) {
    return tr("%1 B").arg(bytes);
  }

  const double kb = bytes / 1024.0;

  if (kb < 1024.0) {
    return tr("%1 KB").arg(kb, 0, 'f', 1);
  }

  const double mb = kb / 1024.0;

  if (mb < 1024.0) {
    return tr("%1 MB").arg(mb, 0, 'f', 1);
  }

  const double gb = mb / 1024.0;

  return tr("%1 GB").arg(gb, 0, 'f', 2);
}

void AnalysisPage::updatePreview(const LinkInfo &link) {
  if (link.state == LinkProcessState::Missing) {
    m_previewLabel->setPixmap({});
    m_previewLabel->setText(tr("Archivo no encontrado"));
    return;
  }

  m_previewLabel->setPixmap({});
  m_previewLabel->setText(tr("Preview\n%1").arg(link.fileName));
}
