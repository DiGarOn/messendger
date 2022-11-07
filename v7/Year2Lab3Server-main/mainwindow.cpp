#include "mainwindow.hpp"
#include <QMenuBar>
#include <QDialog>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    dialog = new Dialog();
    dialog->setWindow(this);
    setCentralWidget(dialog);

    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    QAction *enableAct = new QAction(tr("Enable"));
    connect(enableAct, &QAction::triggered, dialog, &Dialog::Start);
    fileMenu->addAction(enableAct);

    QAction *disableAct = new QAction(tr("Disable"));
    connect(disableAct, &QAction::triggered, dialog, &Dialog::Stop);
    fileMenu->addAction(disableAct);

    QAction *saveLogsAct = new QAction(tr("Save logs as xml"));
    connect(saveLogsAct, &QAction::triggered, dialog, &Dialog::saveLogs);
    fileMenu->addAction(saveLogsAct);

    const QIcon exitIcon = QIcon::fromTheme("application-exit");
    QAction *exitAct = fileMenu->addAction(exitIcon, tr("E&xit"), this, &QWidget::close);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));

    QMenu *settingsMenu = menuBar()->addMenu(tr("&Settings"));

    QAction *networkAct = new QAction(tr("Network"));
    connect(networkAct, &QAction::triggered, dialog, &Dialog::GetIpAndPort);
    settingsMenu->addAction(networkAct);

    setWindowTitle("127.0.0.1:45678 0 users");
}

