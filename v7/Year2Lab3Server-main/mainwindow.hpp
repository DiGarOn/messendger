#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <dialog.h>
#include <QtWidgets/QMainWindow>

class Dialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    Dialog* dialog;
};

#endif // MAINWINDOW_HPP
