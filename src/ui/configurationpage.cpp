#include "configurationpage.h"
#include "configuration/conversiontab.h"
#include "configuration/imageeditingtab.h"
#include "configuration/resolutiontab.h"
#include "configuration/summarypannel.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

ConfigurationPage::ConfigurationPage(QWidget *parent) : QWidget(parent) {
  m_preset.id = QStringLiteral("default");

  m_preset.name = tr("Predeterminado");

  m_preset.builtIn = true;

  setupUi();
  setupConnections();

  updateUiFromPreset();
}

void ConfigurationPage::setupUi() {

  // Configuración de interfaz
  int margin = 12;

  auto *mainLayout = new QVBoxLayout(this);

  mainLayout->setContentsMargins(margin, margin, margin, margin);
  mainLayout->setSpacing(10);

  /* --> Barra de Presets <-- */

  auto *presetLayout = new QHBoxLayout;
  auto *presetLabel = new QLabel(tr("Preset:"), this);
  auto *presetComboBox = new QComboBox(this);

  presetComboBox->addItem(tr("Predeterminado"));
  presetComboBox->setMinimumWidth(200);

  auto *newButton = new QPushButton(tr("Nuevo"), this);
  auto *saveButton = new QPushButton(tr("Guardar"), this);
  auto *saveAsButton = new QPushButton(tr("Guardar como..."), this);
  auto *deleteButton = new QPushButton(tr("Eliminar"), this);

  deleteButton->setEnabled(false); // No debes borrar el preset por default

  presetLayout->addWidget(presetLabel);
  presetLayout->addWidget(presetComboBox);

  // TODO: chequear que 8 sea un valor adecuado
  presetLayout->addSpacing(8);

  presetLayout->addWidget(newButton);
  presetLayout->addWidget(saveButton);
  presetLayout->addWidget(saveAsButton);
  presetLayout->addWidget(deleteButton);
  presetLayout->addStretch();

  mainLayout->addLayout(presetLayout);

  // Separador
  auto *topSeparador = new QFrame(this);
  topSeparador->setFrameShape(QFrame::HLine);
  topSeparador->setFrameShadow(QFrame::Sunken);

  mainLayout->addWidget(topSeparador);

  /*-->Área principal <-- */

  auto contentLayout = new QHBoxLayout;

  contentLayout->setSpacing(margin);

  // TABS --------------------------

  auto *tabs = new QTabWidget(this);

  // auto *resolutionPage = new ResolutionTab(tabs);
  m_resolutionTab = new ResolutionTab(tabs);
  m_imageEditingTab = new ImageEditingTab(tabs);
  m_conversionTab = new ConversionTab(tabs);

  tabs->addTab(m_resolutionTab, tr("Resolución"));
  tabs->addTab(m_imageEditingTab, tr("Edición de imágenes"));
  tabs->addTab(m_conversionTab, tr("Conversión de imágenes"));

  contentLayout->addWidget(tabs, 1);

  // RESUMEN ------------------------

  m_summaryPanel = new SummaryPanel(this);
  contentLayout->addWidget(m_summaryPanel);

  mainLayout->addLayout(contentLayout, 1);

  // ---------------------------------------------------------
  // Separador inferior
  // ---------------------------------------------------------

  auto *bottomSeparator = new QFrame(this);
  bottomSeparator->setFrameShape(QFrame::HLine);
  bottomSeparator->setFrameShadow(QFrame::Sunken);

  mainLayout->addWidget(bottomSeparator);

  // ---------------------------------------------------------
  // Barra de botones
  // ---------------------------------------------------------

  auto *actionLayout = new QHBoxLayout;

  auto *helpButton = new QPushButton(tr("Ayuda"), this);
  auto *analyzeButton = new QPushButton(tr("Analizar documento"), this);
  auto *exitButton = new QPushButton(tr("Salir"), this);

  analyzeButton->setDefault(true);
  analyzeButton->setAutoDefault(true);

  actionLayout->addWidget(helpButton);
  actionLayout->addStretch();
  actionLayout->addWidget(analyzeButton);
  actionLayout->addWidget(exitButton);

  mainLayout->addLayout(actionLayout);

  // ---------------------------------------------------------
  // Signals
  // ---------------------------------------------------------

  connect(analyzeButton, &QPushButton::clicked, this,
          &ConfigurationPage::analyzeDocumentRequested);

  connect(helpButton, &QPushButton::clicked, this,
          &ConfigurationPage::helpRequested);

  connect(exitButton, &QPushButton::clicked, this,
          &ConfigurationPage::exitRequested);
}

void ConfigurationPage::setupConnections() {
  connect(m_resolutionTab, &ResolutionTab::settingsChanged, this,
          &ConfigurationPage::updatePresetFromUi);

  connect(m_summaryPanel, &SummaryPanel::resetRequested, this,
          &ConfigurationPage::resetPreset);
  connect(m_imageEditingTab, &ImageEditingTab::settingsChanged, this,
          &ConfigurationPage::updatePresetFromUi);

  connect(m_conversionTab, &ConversionTab::settingsChanged, this,
          &ConfigurationPage::updatePresetFromUi);
}

void ConfigurationPage::updatePresetFromUi() {
  m_preset.resolution = m_resolutionTab->settings();

  m_preset.imageEditing = m_imageEditingTab->settings();

  m_preset.conversion = m_conversionTab->settings();

  m_summaryPanel->setPreset(m_preset);
}

void ConfigurationPage::updateUiFromPreset() {
  m_resolutionTab->setSettings(m_preset.resolution);
  m_imageEditingTab->setSettings(m_preset.imageEditing);
  m_conversionTab->setSettings(m_preset.conversion);
  m_summaryPanel->setPreset(m_preset);
}

void ConfigurationPage::resetPreset() {
  m_preset.resolution = ResolutionSettings{};
  m_preset.imageEditing = ImageEditingSettings{};
  m_preset.conversion = ConversionSettings{};

  updateUiFromPreset();
}
