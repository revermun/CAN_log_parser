#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <bits/stdc++.h>
#include <cmath>
#include <QVector>
#include <format>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class mainWindow : public QMainWindow
{
    Q_OBJECT

public:
    mainWindow(QWidget *parent = nullptr);
    ~mainWindow();

public slots:
    void browse();
    void parse();
signals:

    void needUpdate();

private:
    Ui::MainWindow *ui;
    QString fileName;
};
#endif // MAINWINDOW_H
