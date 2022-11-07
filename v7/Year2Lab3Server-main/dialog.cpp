#include "dialog.h"
#include "ui_dialog.h"
#include <QMessageBox>
#include <QMenuBar>
#include <QtXml>
#include <QTextStream>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

Dialog::Dialog(QWidget *parent) : QDialog(parent), ui(new Ui::Dialog)
{
    ui->setupUi(this);

    lwUsers = new QListWidget(this);
    lwUsers->hide();

    leHost = new QLineEdit();
    lePort = new QLineEdit();

    leHost->setText("127.0.0.1");
    lePort->setText("45678");

    //создаем сервер. первый параметр стандартный - parent, второй - передадим ссылку на объект виджета, для подключения сигналов от myclient к нему
    _serv = new MyServer(this, this);

    QDialog::connect(this, SIGNAL(messageFromGui(QString,QStringList)), _serv, SLOT(onMessageFromGui(QString,QStringList)));
    connect(_serv, SIGNAL(addLogToGui(QString,QColor)), this, SLOT(onAddLogToGui(QString,QColor)));


    if (_serv->doStartServer(QHostAddress::LocalHost, static_cast<short>(45678)))
    {
        ui->lwLog->insertItem(0, QTime::currentTime().toString()+" server strated at "+_serv->serverAddress().toString()+":"+QString::number(_serv->serverPort()));
        ui->lwLog->item(0)->setForeground(Qt::green);
    }
    else
    {
        ui->lwLog->insertItem(0, QTime::currentTime().toString()+" server not strated at "+_serv->serverAddress().toString()+":"+QString::number(_serv->serverPort()));
        ui->lwLog->item(0)->setForeground(Qt::red);
    }
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::onAddUserToGui(QString name)
{
    lwUsers->addItem(name);
    ui->lwLog->insertItem(0, QTime::currentTime().toString()+" "+name+" joined");
    ui->lwLog->item(0)->setForeground(Qt::green);
    window->setWindowTitle(leHost->text() + ":" + lePort->text() + ' '
                           + QString::number(lwUsers->count()) + " users");
}

void Dialog::onRemoveUserFromGui(QString name)
{
    for (int i = 0; i < lwUsers->count(); ++i)
        if (lwUsers->item(i)->text() == name)
        {
            lwUsers->takeItem(i);
            ui->lwLog->insertItem(0, QTime::currentTime().toString()+" "+name+" left");
            ui->lwLog->item(0)->setForeground(Qt::green);
            break;
        }
    window->setWindowTitle(leHost->text() + ":" + lePort->text() + ' '
                           + QString::number(lwUsers->count()) + " users");
}

void Dialog::onMessageToGui(QString message, QString from, const QStringList &users)
{
    if (users.isEmpty())
        ui->lwLog->insertItem(0, QTime::currentTime().toString()+" message from "+from+": "+message+" to all");
    else
    {
        ui->lwLog->insertItem(0, QTime::currentTime().toString()+" message from "+from+": "+message+" to "+users.join(","));
        ui->lwLog->item(0)->setForeground(Qt::blue);
    }
}

void Dialog::onAddLogToGui(QString string, QColor color)
{
    addToLog(string, color);
}

void Dialog::on_pbStartStop_toggled(bool checked)
{
    if (checked)
    {
        addToLog(" server stopped at "+_serv->serverAddress().toString()+":"+QString::number(_serv->serverPort()), Qt::green);
        window->setWindowTitle("Server is disabled");
        _serv->close();
    }
    else
    {
        QHostAddress addr;
        if (!addr.setAddress(leHost->text()))
        {
            addToLog(" invalid address " + leHost->text(), Qt::red);
            return;
        }
        if (_serv->doStartServer(addr, lePort->text().toInt()))
        {
            addToLog(" server strated at " + leHost->text() + ":" + lePort->text(), Qt::green);
            window->setWindowTitle(leHost->text() + ":" + lePort->text() + ' '
                                   + QString::number(lwUsers->count()) + " users");
        }
        else
        {
            addToLog(" server not strated at " + leHost->text() + ":" + lePort->text(), Qt::red);
        }
    }
}

void Dialog::Start() {
    on_pbStartStop_toggled(false);
}

void Dialog::Stop() {
    on_pbStartStop_toggled(true);
}

void Dialog::addToLog(QString text, QColor color)
{
    ui->lwLog->insertItem(0, QTime::currentTime().toString()+text);
    ui->lwLog->item(0)->setForeground(color);
}

void Dialog::saveLogs() {
    QFileDialog dialog(this);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    if (dialog.exec() != QDialog::Accepted)
        return;

    QFile xmlFile(dialog.selectedFiles().first() + ".xml");
    if (!xmlFile.open(QFile::WriteOnly | QFile::Text ))
    {
        qDebug() << "Already opened or there is another issue";
        xmlFile.close();
        return;
    }
    QTextStream xmlContent(&xmlFile);

    QDomDocument document;

    //make the root element
    QDomElement root = document.createElement("Logs");
    //add it to document
    document.appendChild(root);

    for (int i = 0; i < ui->lwLog->count(); ++i) {
        QListWidgetItem *item = ui->lwLog->item(i);
        QDomElement log = document.createElement("Log");
        log.setAttribute("content", item->text());
        root.appendChild(log);
    }

    xmlContent << document.toString();
}

void Dialog::GetIpAndPort() {
    QDialog* networkDialog = new QDialog(this);

    QGridLayout* layout = new QGridLayout(networkDialog);

    QLabel* ip = new QLabel("IP: ");
    QLabel* port = new QLabel("Port: ");

    QPushButton* closeButton = new QPushButton("Close window", networkDialog);
    connect(closeButton, &QPushButton::clicked, networkDialog, &QDialog::close);

    layout->addWidget(ip, 0, 0);
    layout->addWidget(leHost, 0, 1);
    layout->addWidget(port, 1, 0);
    layout->addWidget(lePort, 1, 1);
    layout->addWidget(closeButton, 2, 0);

    networkDialog->setLayout(layout);

    networkDialog->show();
}

void Dialog::setWindow(MainWindow* w) {
    window = w;
}
