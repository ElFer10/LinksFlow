#pragma once
#include <QMainWindow>

class QStackedWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent=nullptr);
    ~MainWindow() override = default;

private:
    void createInterface();

    QStackedWidget *m_pages = nullptr;

};
