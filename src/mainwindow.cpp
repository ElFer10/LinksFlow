#include "mainwindow.h"
#include "ui/configurationpage.h"

#include <QApplication>
#include <QStackedWidget>
#include <QString>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    constexpr int winWidth{1200}, winHeight{760}, minWinHeight{600}, minWinWidth{900};
    const QString appName{"LinksFlow"};

    createInterface();

    resize(winWidth,winHeight);
    setMinimumSize(minWinWidth, minWinHeight);

    setWindowTitle(appName);
}

void MainWindow::createInterface()
{
    m_pages = new QStackedWidget(this);

    auto *configurationPage = new ConfigurationPage(m_pages);

    m_pages->addWidget(configurationPage);

    setCentralWidget(m_pages);

    connect(configurationPage,&ConfigurationPage::exitRequested, qApp, &QApplication::quit);
}