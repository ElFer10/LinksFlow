#pragma once

#include <QWidget>

class ConfigurationPage : public QWidget
{
    Q_OBJECT

public:
    explicit ConfigurationPage(QWidget *parent = nullptr);

signals:
    void analyzeDocumentRequested();
    void helpRequested();
    void exitRequested();

private:
    void setupUi();

};