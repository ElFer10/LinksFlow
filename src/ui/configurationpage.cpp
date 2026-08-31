#include "configurationpage.h"
#include "configuration/resolutiontab.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

ConfigurationPage::ConfigurationPage(QWidget *parent) : QWidget(parent) {
  setupUi();
}

void ConfigurationPage::setupUi() {

  /*

  Esta es la idea

  ┌───────────────────────────────────────────────────────────────────┐
  │ Preset: [Predeterminado ▼] [Nuevo] [Guardar] [...]                │
  ├───────────────────────────────────────────────────────────────────┤
  │                                                                   │
  │ ┌────────────────────────────────────┐ ┌────────────────────────┐ │
  │ │ Resolución │ Edición │ Conversión  │ │ Resumen                │ │
  │ │                                    │ │ ────────────────────── │ │
  │ │                                    │ │ Preset: Predeterminado │ │
  │ │                                    │ │                        │ │
  │ │                                    │ │ Resolución             │ │
  │ │                                    │ │ ...                    │ │
  │ │                                    │ │                        │ │
  │ │                                    │ │                        │ │
  │ │                                    │ │                        │ │
  │ │                                    │ │                        │ │
  │ │                                    │ │ [Resetear...]          │ │
  │ └────────────────────────────────────┘ └────────────────────────┘ │
  ├───────────────────────────────────────────────────────────────────┤
  │ [Ayuda]                         [Analizar documento]      [Salir] │
  └───────────────────────────────────────────────────────────────────┘

  */

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

  // Separaådor
  auto *topSeparador = new QFrame(this);      // Se crea un frame separador
  topSeparador->setFrameShape(QFrame::HLine); // El frame será una línea
  topSeparador->setFrameShadow(
      QFrame::Sunken); // Establece el tipo de sombra a hundido

  mainLayout->addWidget(topSeparador);

  /*-->Área principal <-- */

  auto contentLayout = new QHBoxLayout;

  contentLayout->setSpacing(margin);

  // TABS --------------------------

  auto *tabs = new QTabWidget(this);

  auto *resolutionPage = new ResolutionTab(tabs);
  auto *editingPage = new QWidget(tabs);
  auto *conversionPage = new QWidget(tabs);

  tabs->addTab(resolutionPage, tr("Resolución"));
  tabs->addTab(editingPage, tr("Edición de imágenes"));
  tabs->addTab(conversionPage, tr("Conversión de imágenes"));

  contentLayout->addWidget(tabs, 1);

  // RESUMEN ------------------------

  auto *summaryFrame = new QFrame(this);

  summaryFrame->setFrameShape(QFrame::StyledPanel);
  summaryFrame->setMinimumWidth(250);
  summaryFrame->setMaximumWidth(300);

  auto *summaryLayout = new QVBoxLayout(summaryFrame);

  auto *summaryTitle = new QLabel(tr("Resumen"), summaryFrame);

  QFont titleFont = summaryTitle->font();
  titleFont.setBold(true);
  summaryTitle->setFont(titleFont);

  summaryLayout->addWidget(summaryTitle);

  auto *summarySeparator = new QFrame(summaryFrame);
  summarySeparator->setFrameShape(QFrame::HLine);

  summaryLayout->addWidget(summarySeparator);

  auto *summaryPlaceholder = new QLabel(tr("Preset: Predeterminado\n\n"
                                           "Resolución efectiva deseada\n"
                                           "Color / Escala de grises: —\n"
                                           "Monochrome: —\n\n"
                                           "Método de optimización\n"
                                           "—\n\n"
                                           "Edición de imágenes\n"
                                           "—\n\n"
                                           "Conversión de imágenes\n"
                                           "—"),
                                        summaryFrame);

  summaryPlaceholder->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  summaryPlaceholder->setWordWrap(true);

  summaryLayout->addWidget(summaryPlaceholder);
  summaryLayout->addStretch();

  auto *resetButton =
      new QPushButton(tr("Resetear la configuración"), summaryFrame);

  summaryLayout->addWidget(resetButton);

  contentLayout->addWidget(summaryFrame);

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
