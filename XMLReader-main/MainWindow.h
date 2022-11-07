#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>

class MainWindow : public QMainWindow
{
    Q_OBJECT
    QListWidget *lwLogs;
public:
    MainWindow(QWidget *parent = nullptr);
private slots:
    void openSlot();
    void closeSlot();
};
#endif // MAINWINDOW_H
