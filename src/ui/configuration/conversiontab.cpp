
#include "conversiontab.h"

#include "conversionformatdelegate.h"
#include "../../models/conversionrulesmodel.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QHeaderView>
#include <QTableView>
#include <QVBoxLayout>

ConversionTab::ConversionTab(
    QWidget *parent
)
    : QWidget(parent)
{
    setupUi();
    setupConnections();
}

void ConversionTab::setupUi()
{
    auto *mainLayout =
        new QVBoxLayout(this);

    mainLayout->setContentsMargins(
        16,
        16,
        16,
        16
    );

    mainLayout->setSpacing(12);

    // ---------------------------------------------------------
    // Master switch
    // ---------------------------------------------------------

    m_conversionCheckBox =
        new QCheckBox(
            tr("Quiero convertir imágenes"),
            this
        );

    mainLayout->addWidget(
        m_conversionCheckBox
    );

    // ---------------------------------------------------------
    // Table
    // ---------------------------------------------------------

    m_table =
        new QTableView(this);

    m_model =
        new ConversionRulesModel(m_table);

    m_table->setModel(m_model);

    auto *formatDelegate =
        new ConversionFormatDelegate(m_table);

    m_table->setItemDelegateForColumn(
        ConversionRulesModel::
            DestinationFormatColumn,
        formatDelegate
    );

    m_table->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );

    m_table->setSelectionMode(
        QAbstractItemView::SingleSelection
    );

    m_table->setAlternatingRowColors(true);

    m_table->verticalHeader()->setVisible(
        false
    );

    m_table->verticalHeader()
        ->setDefaultSectionSize(30);

    m_table->horizontalHeader()
        ->setStretchLastSection(false);

    m_table->horizontalHeader()
        ->setSectionResizeMode(
            ConversionRulesModel::EnabledColumn,
            QHeaderView::ResizeToContents
        );

    m_table->horizontalHeader()
        ->setSectionResizeMode(
            ConversionRulesModel::SourceFormatColumn,
            QHeaderView::Stretch
        );

    m_table->horizontalHeader()
        ->setSectionResizeMode(
            ConversionRulesModel::DestinationFormatColumn,
            QHeaderView::Stretch
        );

    m_table->horizontalHeader()
        ->setSectionResizeMode(
            ConversionRulesModel::OptionsColumn,
            QHeaderView::ResizeToContents
        );

    mainLayout->addWidget(
        m_table,
        1
    );

    m_model->setConversionEnabled(false);
}

void ConversionTab::setupConnections()
{
    connect(
        m_conversionCheckBox,
        &QCheckBox::toggled,
        this,
        [this](bool checked)
        {
            m_model->setConversionEnabled(
                checked
            );

            if (!m_updatingUi) {
                emit settingsChanged();
            }
        }
    );

    connect(
        m_model,
        &ConversionRulesModel::settingsChanged,
        this,
        [this]()
        {
            if (!m_updatingUi) {
                emit settingsChanged();
            }
        }
    );

    connect(
        m_table,
        &QTableView::clicked,
        this,
        [this](const QModelIndex &index)
        {
            if (
                !m_conversionCheckBox
                     ->isChecked()
            ) {
                return;
            }

            if (
                index.column() ==
                ConversionRulesModel::
                    OptionsColumn
            ) {
                emit optionsRequested(
                    index.row()
                );
            }
        }
    );
}

ConversionSettings
ConversionTab::settings() const
{
    return m_model->settings();
}

void ConversionTab::setSettings(
    const ConversionSettings &settings
)
{
    m_updatingUi = true;

    m_conversionCheckBox->setChecked(
        settings.enabled
    );

    m_model->setSettings(settings);

    m_updatingUi = false;
}
