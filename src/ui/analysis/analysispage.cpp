#include "analysispage.h"

#include "../../models/linkstablemodel.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QFormLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QTableView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QtMath>

AnalysisPage::AnalysisPage(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
    setupConnections();
    clearInspector();
}

void AnalysisPage::setupUi()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    auto *title = new QLabel(tr("Enlaces del documento"), this);

    QFont titleFont = title->font();
    titleFont.setBold(true);
    titleFont.setPointSizeF(titleFont.pointSizeF() + 2.0);
    title->setFont(titleFont);

    mainLayout->addWidget(title);

    auto *description = new QLabel(
        tr("Seleccione los enlaces que desea procesar."),
        this
    );

    mainLayout->addWidget(description);

    auto *splitter = new QSplitter(Qt::Vertical, this);

    // Tabla
    m_table = new QTableView(splitter);

    m_model = new LinksTableModel(this);

    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_model);
    m_proxyModel->setSortCaseSensitivity(
        Qt::CaseInsensitive
    );

    m_table->setModel(m_proxyModel);

    m_table->setSelectionBehavior(
        QAbstractItemView::SelectRows
    );

    m_table->setSelectionMode(
        QAbstractItemView::SingleSelection
    );

    m_table->setAlternatingRowColors(true);
    m_table->setSortingEnabled(true);
    m_table->setEditTriggers(
        QAbstractItemView::SelectedClicked |
        QAbstractItemView::DoubleClicked
    );

    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(28);

    auto *header = m_table->horizontalHeader();

    header->setSectionResizeMode(
        LinksTableModel::ProcessColumn,
        QHeaderView::ResizeToContents
    );

    header->setSectionResizeMode(
        LinksTableModel::PageColumn,
        QHeaderView::ResizeToContents
    );

    header->setSectionResizeMode(
        LinksTableModel::NameColumn,
        QHeaderView::Stretch
    );

    header->setSectionResizeMode(
        LinksTableModel::TypeColumn,
        QHeaderView::ResizeToContents
    );

    header->setSectionResizeMode(
        LinksTableModel::ColorColumn,
        QHeaderView::ResizeToContents
    );

    header->setSectionResizeMode(
        LinksTableModel::SizeColumn,
        QHeaderView::ResizeToContents
    );

    header->setSectionResizeMode(
        LinksTableModel::EffectiveResolutionColumn,
        QHeaderView::ResizeToContents
    );

    header->setSectionResizeMode(
        LinksTableModel::IccProfileColumn,
        QHeaderView::ResizeToContents
    );

    splitter->addWidget(m_table);

    // Inspector
    auto *inspector = new QWidget(splitter);
    auto *inspectorLayout = new QFormLayout(inspector);

    inspectorLayout->setContentsMargins(8, 8, 8, 8);

    m_fileNameValue = new QLabel(inspector);
    m_pathValue = new QLabel(inspector);
    m_actualResolutionValue = new QLabel(inspector);
    m_effectiveResolutionValue = new QLabel(inspector);
    m_scaleValue = new QLabel(inspector);
    m_rotationValue = new QLabel(inspector);
    m_flipValue = new QLabel(inspector);

    m_pathValue->setWordWrap(true);
    m_pathValue->setTextInteractionFlags(
        Qt::TextSelectableByMouse
    );

    inspectorLayout->addRow(
        tr("Archivo:"),
        m_fileNameValue
    );

    inspectorLayout->addRow(
        tr("Ubicación:"),
        m_pathValue
    );

    inspectorLayout->addRow(
        tr("Resolución real:"),
        m_actualResolutionValue
    );

    inspectorLayout->addRow(
        tr("Resolución efectiva:"),
        m_effectiveResolutionValue
    );

    inspectorLayout->addRow(
        tr("Escala:"),
        m_scaleValue
    );

    inspectorLayout->addRow(
        tr("Rotación:"),
        m_rotationValue
    );

    inspectorLayout->addRow(
        tr("Volteado:"),
        m_flipValue
    );

    splitter->addWidget(inspector);

    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter, 1);

    // Barra inferior
    auto *buttonLayout = new QHBoxLayout();

    m_exitButton = new QPushButton(
        tr("Salir"),
        this
    );

    m_backButton = new QPushButton(
        tr("Cancelar"),
        this
    );

    m_processButton = new QPushButton(
        tr("Procesar"),
        this
    );

    m_processButton->setDefault(true);

    buttonLayout->addWidget(m_exitButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_backButton);
    buttonLayout->addWidget(m_processButton);

    mainLayout->addLayout(buttonLayout);
}

void AnalysisPage::setupConnections()
{
    connect(
        m_exitButton,
        &QPushButton::clicked,
        this,
        &AnalysisPage::exitRequested
    );

    connect(
        m_backButton,
        &QPushButton::clicked,
        this,
        &AnalysisPage::backRequested
    );

    connect(
        m_processButton,
        &QPushButton::clicked,
        this,
        &AnalysisPage::processRequested
    );

    connect(
        m_table->selectionModel(),
        &QItemSelectionModel::currentRowChanged,
        this,
        [this](
            const QModelIndex &current,
            const QModelIndex &
        )
        {
            if (!current.isValid()) {
                clearInspector();
                return;
            }

            updateInspector(current);
        }
    );
}

void AnalysisPage::setLinks(
    const QList<LinkInfo> &links
)
{
    m_model->setLinks(links);

    if (m_proxyModel->rowCount() > 0) {
        const QModelIndex first =
            m_proxyModel->index(0, 0);

        m_table->setCurrentIndex(first);
        m_table->selectRow(0);

        updateInspector(first);
    } else {
        clearInspector();
    }
}

void AnalysisPage::updateInspector(
    const QModelIndex &proxyIndex
)
{
    if (!proxyIndex.isValid()) {
        clearInspector();
        return;
    }

    const QModelIndex sourceIndex =
        m_proxyModel->mapToSource(proxyIndex);

    const LinkInfo link =
        m_model->linkAt(sourceIndex.row());

    m_fileNameValue->setText(
        link.fileName
    );

    m_pathValue->setText(
        link.filePath
    );

    m_actualResolutionValue->setText(
        formatResolution(
            link.actualResolution
        )
    );

    m_effectiveResolutionValue->setText(
        formatResolution(
            link.effectiveResolution
        )
    );

    m_scaleValue->setText(
        formatScale(
            link.scale
        )
    );

    m_rotationValue->setText(
        tr("%1°")
            .arg(
                link.rotation,
                0,
                'f',
                1
            )
    );

    m_flipValue->setText(
        formatFlip(
            link.flip
        )
    );
}

void AnalysisPage::clearInspector()
{
    m_fileNameValue->setText(QStringLiteral("—"));
    m_pathValue->setText(QStringLiteral("—"));
    m_actualResolutionValue->setText(QStringLiteral("—"));
    m_effectiveResolutionValue->setText(QStringLiteral("—"));
    m_scaleValue->setText(QStringLiteral("—"));
    m_rotationValue->setText(QStringLiteral("—"));
    m_flipValue->setText(QStringLiteral("—"));
}

QString AnalysisPage::formatResolution(
    const Resolution2D &resolution
) const
{
    constexpr double tolerance = 0.5;

    if (
        qAbs(
            resolution.x -
            resolution.y
        ) < tolerance
    ) {
        return tr("%1 ppi")
            .arg(
                qRound(resolution.x)
            );
    }

    return tr("%1 × %2 ppi")
        .arg(qRound(resolution.x))
        .arg(qRound(resolution.y));
}

QString AnalysisPage::formatScale(
    const Scale2D &scale
) const
{
    constexpr double tolerance = 0.01;

    if (
        qAbs(
            scale.horizontal -
            scale.vertical
        ) < tolerance
    ) {
        return tr("%1 %")
            .arg(
                scale.horizontal,
                0,
                'f',
                2
            );
    }

    return tr("%1 % × %2 %")
        .arg(
            scale.horizontal,
            0,
            'f',
            2
        )
        .arg(
            scale.vertical,
            0,
            'f',
            2
        );
}

QString AnalysisPage::formatFlip(
    const FlipState &flip
) const
{
    if (
        flip.horizontal &&
        flip.vertical
    ) {
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
