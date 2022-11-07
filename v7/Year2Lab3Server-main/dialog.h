#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QDebug>
#include <QtGui>
#include <QtCore>
#include <QMainWindow>
#include <QListWidget>
#include "myserver.h"
#include "mainwindow.hpp"

namespace Ui {
    class Dialog;
}

class MainWindow;

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();
    void setWindow(MainWindow* w);

private:
    bool started = true;
    Ui::Dialog *ui;
    MyServer *_serv;
    void addToLog(QString text, QColor color);
    QListWidget* lwUsers;
    QLineEdit* leHost;
    QLineEdit* lePort;
    MainWindow* window;

signals:
    void messageFromGui(QString message, const QStringList &users);

public slots:
    void onAddUserToGui(QString name);
    void onRemoveUserFromGui(QString name);
    void onMessageToGui(QString message, QString from, const QStringList &users);
    void onAddLogToGui(QString string, QColor color);

    void Start();
    void Stop();
    void on_pbStartStop_toggled(bool checked);
    void saveLogs();
    void GetIpAndPort();
};

#endif // DIALOG_H
