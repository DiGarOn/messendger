#include "dialog.h"
#include "ui_mainwindow.h"

#include <QtGui>
#include <QDebug>
#include <QMenuBar>
#include <QLineEdit>
#include <QLabel>
#include <QFileDialog>
#include <QtXml>
#include <fstream>
#include <QColorDialog>
#include <QToolBar>
#include <QFontDialog>
#include <QVBoxLayout>
#include <QPushButton>

Dialog::Dialog(QWidget *parent) :QMainWindow(parent),ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->lwLog->setIconSize(QSize(320, 240));

    _name = "Unknown_Hedgehog";
    qDebug() << QDir::currentPath();
    avatar.load(QDir::currentPath() + "/New_user.png");

    _sok = new QTcpSocket(this);

    _ip = QHostAddress("127.0.0.1");
    _port = 45678;

    _status = "online";

    setWindowTitle("offline No connection");

    QMenu *file = menuBar()->addMenu("&File");
    connectAct = file->addAction("Connect to Server",this,SLOT(connectionButton()));
    disconnectAct = file->addAction("Disconnect",this,SLOT(disconnectButton()));
    disconnectAct->setEnabled(false);
    file->addAction("Save to XML",this,SLOT(saveToXML()));
    file->addAction("Quit",QApplication::instance(),SLOT(quit()));

    QMenu *settings = menuBar()->addMenu("&Settings");
    serverAct=settings->addAction("Server: "+_ip.toString()+':'+QString::number(_port),this,SLOT(changeServer()));
    QAction *autoFileAccept = settings->addAction("Automatic File Accept");
    autoFileAccept->setCheckable(true);
    autoFileAccept->setChecked(true);
    connect(autoFileAccept,SIGNAL(toggled(bool)),this,SLOT(autoFileAcceptSlot(bool)));

    _name = "New_user" + QString::number(QRandomGenerator::global()->generate() % 100);
    settings->addAction("Change name: "+_name,this,SLOT(changeName()));
    userPhoto = settings->addAction("Choose avatar",this,SLOT(chooseAvatar()));
    QMenu *status=settings->addMenu("Status");
    onlineSt = status->addAction("Online",this,SLOT(changeStatus()));
    onlineSt->setCheckable(true);
    goneSt = status->addAction("Gone",this,SLOT(changeStatus()));
    goneSt->setCheckable(true);
    doNotDisturbSt=status->addAction("Do not disturb",this,SLOT(changeStatus()));
    doNotDisturbSt->setCheckable(true);
    ownSt = status->addAction("Own status",this,SLOT(changeStatus()));
    ownSt->setCheckable(true);

    CBC = new QAction("Change Background Color",this);
    MC = new QAction("Message customizaton",this);
    SBD = new QAction("Status bar display",this);
    SBD->setCheckable(true);

    readSettings();

    SBD->setChecked(shownStatusBar);
    IPAD = new QAction("IP adress display",this);
    IPAD->setCheckable(true);
    IPAD->setChecked(true);

    TD = new QAction("Time Dispaly",this);
    TD->setCheckable(true);
    TD->setChecked(true);

    connect(CBC, SIGNAL(triggered()),this,SLOT(ChangeBackgroundColor()));
    connect(MC,  SIGNAL(triggered()),this,SLOT(MessageCustomization()));
    connect(SBD, SIGNAL(triggered()),this,SLOT(StatusBarDispaly()));
    connect(IPAD,SIGNAL(triggered()),this,SLOT(updateMessages()));
    connect(TD,  SIGNAL(triggered()),this,SLOT(updateMessages()));

    QMenu *view = menuBar()->addMenu("&View");
    view->addAction(CBC);
    view->addAction(MC);
    view->addAction(SBD);
    view->addAction(IPAD);
    view->addAction(TD);

    QMenu *help = menuBar()->addMenu("&Help");
    help->addAction("About",this,SLOT(aboutMe()));

    StatusBarDispaly();

    connect(_sok, SIGNAL(readyRead()), this, SLOT(onSokReadyRead()));
    connect(_sok, SIGNAL(connected()), this, SLOT(onSokConnected()));
    connect(_sok, SIGNAL(disconnected()), this, SLOT(onSokDisconnected()));
    connect(_sok, SIGNAL(error(QAbstractSocket::SocketError)),this, SLOT(onSokDisplayError(QAbstractSocket::SocketError)));
    connect(ui->pbSend,SIGNAL(clicked()),this,SLOT(chooseSender()));
    //    connect(ui->pteMessage,SIGNAL(blockCountChanged(int)),ui->pbSend,SLOT(click()));

    ui->lwLog->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->lwLog, SIGNAL(customContextMenuRequested(const QPoint &)),
        this, SLOT(ProvideContextMenu(const QPoint &)));

    ui->lwUsers->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->lwUsers, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(ProvideRightContextMenu(const QPoint &)));

    {
        QMenu *popupFAR = new QMenu(this);
        sendImageAction = popupFAR->addAction(tr("Send image"), this, &Dialog::chooseImage);
        sendFTextAction = popupFAR->addAction(tr("Formated text"), this, &Dialog::formatText);
        sendAction = popupFAR->addAction(tr("Send"), this, &Dialog::setUpSend);
        sendAction->setVisible(false);

        ui->pbFont->setVisible(false);
        ui->pbColour->setVisible(false);

        connect(ui->pbFont, &QPushButton::clicked, this, &Dialog::chooseFont);
        connect(ui->pbColour, &QPushButton::clicked, this, &Dialog::chooseColor);
        ui->pbSend->setMenu(popupFAR);
        ui->pbSend->setPopupMode(QToolButton::MenuButtonPopup);

        ui->pbSend->setEnabled(false);
    }

    newMessage.setMedia(QUrl::fromLocalFile("/Users/dmitriygarkin/Documents/PL/qt/mixkit-doorbell-single-press-333.wav"));
    newMessage.setVolume(50);
}

Dialog::~Dialog()
{
    delete ui;
}
//Обработка возможных ошибок
void Dialog::onSokDisplayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError)
    {
        case QAbstractSocket::RemoteHostClosedError:
            break;
        case QAbstractSocket::HostNotFoundError:
            QMessageBox::information(this, "Error", "The host was not found");
            break;
        case QAbstractSocket::ConnectionRefusedError:
            QMessageBox::information(this, "Error", "The connection was refused by the peer.");
            break;
        default:
            QMessageBox::information(this, "Error", "The following error occurred: "+_sok->errorString());
    }
}
//тут обрабатываются данные от сервера
void Dialog::onSokReadyRead()
{  
    QDataStream in(_sok);

    //если считываем новый блок первые 2 байта это его размер
    if (_blockSize == 0)
    {

        if (_sok->bytesAvailable() < (int)sizeof(quint64))      //qint16 - как раз два байта и есть, далее переводим в int
            return;                                             //если пришло меньше 2 байт ждем пока будет 2 байта

        //считываем размер (2 байта)
        in >> _blockSize;
    }

    //ждем пока блок прийдет полностью
    if (_sok->bytesAvailable() < _blockSize)
        return;
    else
        //можно принимать новый блок
        _blockSize = 0;
    //3 байт - команда серверу
    quint8 command;
    in >> command;
    qDebug() << "Received command " << command;

    //команды см. в хедере сервака
    switch (command)
    {
        //сервер отправит список пользователей, если авторизация пройдена, в таком случае третий байт равен константе MyClient::comUsersOnline
        case MyClient::comAutchSuccess:
        {
            ui->pbSend->setEnabled(true);
//            AddToLog("Enter as "+_name,Qt::green);
        }
        break;
        case MyClient::comUsersOnline:
        {
//          AddToLog("Received user list "+_name,Qt::green);
            ui->pbSend->setEnabled(true);

            int usersNumber;
            in >> usersNumber;

            QString userName;
            QPixmap ava;

            for (int i = 0; i< usersNumber; ++i) {
                in >> userName;
                in >> ava;
                QListWidgetItem* listItem = new QListWidgetItem( QIcon(ava), userName );
                ui->lwUsers->addItem( listItem );
            }

            //обновляем гуи
            ui->lwUsers->setIconSize( QSize( 24, 24 ) );
        }
        break;
        //общее сообщение от сервера
        case MyClient::comPublicServerMessage:
        {
            //считываем и добавляем в лог
            QString message;
            in >> message;
            AddToLog("[PublicServerMessage]: "+message, Qt::red);
        }
        break;
        case MyClient::comMessageToAll:
        {
            QString user;
            in >> user;
            QString ip;
            in >> ip;
            QString time;
            in >> time;
            quint8 type;
            in >> type;
            if (type == comPlainText) { //plain text
                QString message;
                in >> message;
                Message msg(ip, time, _name, type, message);
                messages.push_back(msg);
                QFont f("Calibri");
                AddToLogRichText(message, currentTextColor, f);
                AddToLogRichText(time + ip + "["+user+"]: ", currentNameColor, f);
            }
            else if (type == comImage) {
                QImage img;
                in >> img;
                Message msg(ip, time, _name, type, img);
                messages.push_back(msg);
                AddToLogImage(img);
            }
            else if (type == comRichText) {
                QString colorName, fontKey, message;
                in >> colorName >> fontKey >> message;
                Message msg(ip, time, _name, type, colorName + '\n' + fontKey + '\n' + message);
                messages.push_back(msg);
                AddToLogRichText(message, colorName, fontKey);
                AddToLog(time + ip + "["+user+"]: ");
                updateMessages();
            }
            //do smth
        }
        break;
        case MyClient::comMessageToUsers:
        {
            QString user;
            in >> user;
            QString message;
            in >> message;
            AddToLog('['+user+"](private): "+message, Qt::blue);
        }
        break;
        case MyClient::comPrivateServerMessage:
        {
            QString message;
            in >> message;
            AddToLog("[PrivateServerMessage]: "+message, Qt::red);
        }
        break;
        case MyClient::comUserJoin:
        {
            QString name;
            QPixmap ava;
            in >> name;
            in >> ava;
            ui->lwUsers->setIconSize( QSize( 24, 24 ) );

            QListWidgetItem* listItem = new QListWidgetItem( QIcon(ava), name );

            //listItem->setFlags( Qt::ItemIsEditable | Qt::ItemIsEnabled );
            ui->lwUsers->addItem( listItem );

//            AddToLog(name+" joined", Qt::green);
        }
        break;
        case MyClient::comUserLeft:
        {
            QString name;
            in >> name;
            for (int i = 0; i < ui->lwUsers->count(); ++i)
                if (ui->lwUsers->item(i)->text() == name)
                {
                    ui->lwUsers->takeItem(i);
                    AddToLog(name+" left", Qt::green);
                    break;
                }
        }
        break;
        case MyClient::comErrNameInvalid:
        {
            QMessageBox::information(this, "Error", "This name is invalid.");
            _sok->disconnectFromHost();
        }
        break;
        case MyClient::comErrNameUsed:
        {
            QMessageBox::information(this, "Error", "This name is already used.");
            _sok->disconnectFromHost();
        }
        break;
        case MyClient::comGetStatus:
        {
            QByteArray block;
            QDataStream out(&block, QIODevice::WriteOnly);
            out << (quint64)0;
            out << MyClient::comGetStatus << _status;
            out.device()->seek(0);
            out << (quint64)(block.size() - sizeof(quint64));
            _sok->write(block);
        }
        break;
        case MyClient::comAskForInformation: {
            QString tmp;
            in >> tmp;
            selectedUserInformation.push_back(tmp);
            in >> tmp;
            selectedUserInformation.push_back(tmp);
            in >> tmp;
            selectedUserInformation.push_back(tmp);


            QDialog *dialog = new QDialog(this);
            dialog->setModal(true);
            dialog->setWindowTitle("Full image");

            QGridLayout *l = new QGridLayout(dialog);

            QLabel *ipLb = new QLabel("Ip: " + selectedUserInformation.at(0), dialog);
            l->addWidget(ipLb, 0, 0);

            QLabel *dateLb = new QLabel("Date of connetion: " + selectedUserInformation.at(1), dialog);
            l->addWidget(dateLb, 1, 0);

            QLabel *statusLb = new QLabel("User's status: " + selectedUserInformation.at(2), dialog);
            l->addWidget(statusLb, 2, 0);

            QPushButton *acc = new QPushButton("Close",dialog);
            connect(acc,SIGNAL(clicked()),dialog,SLOT(accept()));
            l->addWidget(acc, 3, 0);
            dialog->setLayout(l);
            dialog->exec();

            selectedUserInformation.clear();
        }
        break;
        case MyClient::comFile: {
            QString ip;
            in >> ip;

            QString time;
            in >> time;

            QString fileName;
            in >> fileName;

            QString userFromName;
            in >> userFromName;

            if(!autoFileAcception){
                QByteArray file = _sok->readAll();

                QDialog *mc = new QDialog(this);
                mc->setModal(false);
                mc->setWindowTitle("Принять файл?");
                mc->setMinimumSize(240,240);
                QPalette pal = mc->palette();
                pal.setColor(mc->backgroundRole(), Qt::white);
                mc->setPalette(pal);
                QVBoxLayout *l = new QVBoxLayout(mc);

                QLabel *Message = new QLabel("Recieved file: " + fileName.section("/", -1) + " from " + userFromName + " Size: " +
                                                 QString::number(file.size()) + " bytes", mc);
                l->addWidget(Message);

                QPushButton *acc = new QPushButton("Accept", mc);
                connect(acc, SIGNAL(clicked()), mc, SLOT(accept()));


                QPushButton *decline = new QPushButton("Decline", mc);
                connect(decline, SIGNAL(clicked()), mc, SLOT(reject()));

                l->addWidget(Message);
                l->addWidget(acc);
                l->addWidget(decline);
                mc->setLayout(l);
                mc->setModal(false);
                if(mc->exec() == QDialog::Accepted) {
                    checkAgain:

                    QFile File(fileName);
                    QFileInfo FileInfo(File);
                    QString filePath = "/home/user/Documents"; // your file path for receiving

                    QString newName = "";

                    //
                    QString targetStr = FileInfo.suffix();
                    QFileInfoList hitList;
                    QString directory = filePath + "/";
                    QDirIterator it(directory, QDirIterator::Subdirectories);

                    while (it.hasNext()) {
                        QString FileName = it.next();
                        QFileInfo file(FileName);

                        if (file.isDir()) {
                            continue;
                        }

                        if (file.fileName().contains(targetStr, Qt::CaseInsensitive)) {
                            hitList.append(file);

                        }
                    }

                    for (auto &file : hitList) {
                        if(file.baseName()==FileInfo.baseName()){
                            newName = FileInfo.baseName() + "1." + FileInfo.suffix();

                        }
                    }
                    //
                    if(newName!=""){
                        fileName = newName;
                        goto checkAgain;
                    }
                    else{
                        fileName = fileName.section("/", -1);
                    }
                    QFile target(filePath + "/" + fileName);

                    if (!target.open(QIODevice::WriteOnly)) {
                        qDebug() << "Can't open file for written";
                        return;
                    }
                    target.write(file);

                    target.close();

                    QCryptographicHash *hash = new QCryptographicHash(QCryptographicHash::Md5);
                    hash->addData(file);
                    QString pass = QString(hash->result().toHex());

                    struct Message msg(ip, time, userFromName, 4, fileName + ' ' + pass);
                    messages.push_back(msg);
                }
            }
            else        //если стоит автомат
            {
                QByteArray file = _sok->readAll();

            checkAgainNoDialog:
                //save with different name if needed
                QFile File(fileName);
                QFileInfo FileInfo(File);
                QString filePath = "/home/user/Documents"; // your file path for receiving

                QString newName = "";

                //
                QString targetStr = FileInfo.suffix();
                QFileInfoList hitList;
                QString directory = filePath + "/";
                QDirIterator it(directory, QDirIterator::Subdirectories);

                while (it.hasNext()) {
                    QString FileName = it.next();
                    QFileInfo file(FileName);

                    if (file.isDir()) {
                        continue;
                    }

                    if (file.fileName().contains(targetStr, Qt::CaseInsensitive)) {
                        hitList.append(file);

                    }
                }

                for (auto &file : hitList) {
                    if(file.baseName()==FileInfo.baseName()){
                        newName = FileInfo.baseName() + "1." + FileInfo.suffix();

                    }
                }
                //
                if(newName!=""){
                    fileName = newName;
                    goto checkAgainNoDialog;
                }
                else{
                    fileName = fileName.section("/", -1);
                }
                QFile target(filePath + "/" + fileName);

                if (!target.open(QIODevice::WriteOnly)) {
                    qDebug() << "Can't open file for written";
                    return;
                }
                target.write(file);

                target.close();

                AddToLog("Recieved file " + fileName.section("/", -1) + " from user " + userFromName);

                QCryptographicHash *hash = new QCryptographicHash(QCryptographicHash::Md5);
                hash->addData(file);
                QString pass = QString(hash->result().toHex());

                Message msg(ip, time, userFromName, 4, fileName + ' ' + pass);
                messages.push_back(msg);
            }

            QByteArray block;
            QDataStream out(&block, QIODevice::WriteOnly);
            out << (quint64)0;
            out << MyClient::comFileProgress;

            out << userFromName;
            out << 100;

            out.device()->seek(0);
            out << (quint64)(block.size() - sizeof(quint64));
            _sok->write(block);
        }
        break;
        case MyClient::comFileProgress: {
            int progress;
            in >> progress;
            setProgressBar(progress);
        }
        break;
    }
}

void Dialog::onSokConnected()
{
    connectAct->setEnabled(false);
    disconnectAct->setEnabled(true);
    _blockSize = 0;

    if(_status=="online")
        onlineSt->toggle();
    else if(_status=="gone")
        goneSt->toggle();
    else if(_status=="do not disturb")
        doNotDisturbSt->toggle();
    else
        ownSt->toggle();
    setWindowTitle(_status+' '+_sok->peerAddress().toString()+":"+QString::number(_sok->peerPort()));

//    AddToLog("Connected to"+_sok->peerAddress().toString()+":"+QString::number(_sok->peerPort()),Qt::green);

    //после подключения следует отправить запрос на авторизацию
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    //резервируем 8 байт для размера блока. Класс MyClient используется в реализации сервера, но тут используем статические члены этого класса - константы команд
    //третий байт - команда
    out << (quint64)0;
    out << (quint8)MyClient::comAutchReq;
    out << _name; //ui->leName->text();
    out << avatar.toImage();
    //возваращаемся в начало
    out.device()->seek(0);
    //вписываем размер блока на зарезервированное место
    out << (quint64)(block.size() - sizeof(quint64));
    _sok->write(block);
}

void Dialog::chooseAvatar() {
    QFileDialog openDialog(this, tr("Open file..."));
    openDialog.setAcceptMode(QFileDialog::AcceptOpen);
    openDialog.setFileMode(QFileDialog::ExistingFile);
    //openDialog.setMimeTypeFilters({"text/plain"});
    if(openDialog.exec() != QDialog::Accepted)
        return;
    QString fn = openDialog.selectedFiles().constFirst();
    avatar.load(fn);
}

void Dialog::onSokDisconnected()
{
    connectAct->setEnabled(true);
    disconnectAct->setEnabled(false);
    ui->pbSend->setEnabled(false);
    ui->lwUsers->clear();

    onlineSt->setChecked(false);
    goneSt->setChecked(false);
    doNotDisturbSt->setChecked(false);
    ownSt->setChecked(false);
    setWindowTitle("offline No connection");

//    AddToLog("Disconnected from"+_sok->peerAddress().toString()+":"+QString::number(_sok->peerPort()), Qt::green);
}

//по нажатию кнопки подключаемся к северу, отметим, что connectToHost() возвращает
//тип void, потому, что это асинхронный вызов и в случае ошибки будет вызван слот onSokDisplayError
void Dialog::connectionButton()
{
    _sok->connectToHost(_ip, _port);
}

void Dialog::disconnectButton()
{
    _sok->disconnectFromHost();
}

void Dialog::on_cbToAll_clicked()
{
    if (ui->cbToAll->isChecked())
        ui->pbSend->setText("Send To All");
    else
        ui->pbSend->setText("Send To Selected");
}

void Dialog::askForInformation(QString name) {
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint64)0;
    //отправляется команда

    out << (quint8)MyClient::comAskForInformation;
    out << _name << name;
    out.device()->seek(0);
    out << (quint64)(block.size() - sizeof(quint64));
    _sok->write(block);
}

void Dialog::sendMsg()
{
    if(ui->pbSend->text() == "Send")
    {
        sendAction->setVisible(false);
        sendImageAction->setVisible(true);
        sendFTextAction->setVisible(true);
        ui->pbFont->setVisible(false);
        ui->pbColour->setVisible(false);
    }
    if(ui->pbSend->text() == "Send image")
    {
        sendAction->setVisible(true);
        sendImageAction->setVisible(false);
        sendFTextAction->setVisible(true);
        ui->pbFont->setVisible(false);
        ui->pbColour->setVisible(false);
    }
    if(ui->pbSend->text() == "Formated send")
    {
        sendAction->setVisible(true);
        sendImageAction->setVisible(true);
        sendFTextAction->setVisible(false);
    }

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint64)0;

    if (sendingFile) {//файл
        out << MyClient::comFile;

        out << QTime::currentTime().toString() + ' ';

        out << chosenUser;
        out << chosenFileName;

        out << _name;

        block.append(chosenFile);

        out.device()->seek(0);
        // This difference appear because of we send file name
        out << (quint64)(block.size() - sizeof(quint64));

        qint64 x = 0;
        while (x < block.size()) {
            qint64 y = _sok->write(block);
            x += y;
            qDebug() << x/block.size()*100;
            //qDebug() << x;    // summary size you send, so you can check recieved and replied sizes
        }
        return;
    }

    //отправляется команда
    if (ui->cbToAll->isChecked())
        out << (quint8)MyClient::comMessageToAll;
    else
    {
        out << (quint8)MyClient::comMessageToUsers;
        QString s;
        foreach (QListWidgetItem *i, ui->lwUsers->selectedItems())
            s += i->text()+",";
        s.remove(s.length()-1, 1);
        out << s;
    }

    //отправляется content
    //ip и имя добавятся на клиенте
    out << QTime::currentTime().toString() + ' ';
    if (sendingFormatedText) {//текст с форматированием
        out << comRichText;
        out << ui->pteMessage->palette().color(QPalette::Text).name() << ui->pteMessage->font().family() << ui->pteMessage->document()->toPlainText();
    }
    else if (sendingImage) {//картинка
        out << comImage;
        out << image.toImage();
    }
    else //если отправляем обычный текст
    {
        out << comPlainText;
        out << ui->pteMessage->document()->toPlainText();
    }
    //возврат в начало
    out.device()->seek(0);
    //пишем размер сообщения
    out << (quint64)(block.size() - sizeof(quint64));
    _sok->write(block);
    ui->pteMessage->clear();
}

void Dialog::AddToLogImage(QImage img) {
    QListWidgetItem* item = new QListWidgetItem(QIcon(QPixmap::fromImage(img).scaled(320, 240)), "");
    ui->lwLog->insertItem(0, item);
    if (_status != "do not disturb")
        newMessage.play();
}

void Dialog::AddToLog(QString text, QColor color)
{
    ui->lwLog->insertItem(0, text);
    ui->lwLog->item(0)->setForeground(color);

    if (_status != "do not disturb")
        newMessage.play();
}

void Dialog::AddToLogRichText(QString text, QColor color, QFont font) {
    ui->lwLog->insertItem(0, text);
    ui->lwLog->item(0)->setForeground(color);
    ui->lwLog->item(0)->setFont(font);
    if (_status != "do not disturb")
        newMessage.play();
}

void Dialog::changeServer()
{
    QDialog *dlgServer = new QDialog(this);
    dlgServer->setModal(true);
    dlgServer->setFixedSize(136,71); // Можно поменять
    dlgServer->setWindowTitle("Change server");
    QGridLayout *l = new QGridLayout(dlgServer);
    QLineEdit *leIP = new QLineEdit(dlgServer);
    leIP->setText(_ip.toString());
    l->addWidget(leIP,0,0);
    QLineEdit *lePort = new QLineEdit(dlgServer);
    lePort->setText(QString::number(_port));
    l->addWidget(lePort,0,1);
    QPushButton *accBut = new QPushButton("Accept",dlgServer);
    connect(accBut,SIGNAL(clicked()),dlgServer,SLOT(accept()));
    l->addWidget(accBut,1,0,1,2);
    dlgServer->setLayout(l);
    if(dlgServer->exec())
    {
        QRegExp expr("[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}");
        if(expr.exactMatch(leIP->text()))
        {
            QStringList nums=leIP->text().split('.');
            foreach(QString str, nums)
            {
                if(!(str.toInt()<256))
                    return;
            }
            bool ok;
            quint16 port = lePort->text().toUShort(&ok);
            if(!ok)
                return;
            serverAct->setText("Server: "+leIP->text()+':'+lePort->text());
            _ip=leIP->text();
            _port=port;
        }
    }
}

void Dialog::autoFileAcceptSlot(bool b)
{
    autoFileAcception=b;
}

void Dialog::changeName()
{
    QDialog *dlgServer = new QDialog(this);
    dlgServer->setModal(true);
    dlgServer->setFixedSize(136,71); // Можно поменять
    dlgServer->setWindowTitle("Change server");
    QGridLayout *l = new QGridLayout(dlgServer);
    QLineEdit *leName = new QLineEdit(dlgServer);
    leName->setText(_name);
    l->addWidget(leName);
    QPushButton *accBut = new QPushButton("Accept",dlgServer);
    connect(accBut,SIGNAL(clicked()),dlgServer,SLOT(accept()));
    l->addWidget(accBut,1,0,1,2);
    dlgServer->setLayout(l);
    if(dlgServer->exec())
    {
        QRegExp expr("[^@!\"\\\\'&|\\*]+");
        if(expr.exactMatch(leName->text()))
        {
            _name=leName->text();
            ((QAction*)sender())->setText("Change name: "+_name);
        }
    }
}

void Dialog::changeStatus()
{
    if(!(_sok->state()==QTcpSocket::ConnectedState))
    {
        ((QAction*)sender())->setChecked(!((QAction*)sender())->isChecked());
        return;
    }
    if(sender()==onlineSt)
    {
        onlineSt->setChecked(true);
        goneSt->setChecked(false);
        doNotDisturbSt->setChecked(false);
        ownSt->setChecked(false);
        _status="online";
    }
    else if(sender()==goneSt)
    {
        onlineSt->setChecked(false);
        goneSt->setChecked(true);
        doNotDisturbSt->setChecked(false);
        ownSt->setChecked(false);
        _status="gone";
    }
    else if(sender()==doNotDisturbSt)
    {
        onlineSt->setChecked(false);
        goneSt->setChecked(false);
        doNotDisturbSt->setChecked(true);
        ownSt->setChecked(false);
        _status="do not disturb";
    }
    else if(sender()==ownSt)
    {
        onlineSt->setChecked(false);
        goneSt->setChecked(false);
        doNotDisturbSt->setChecked(false);
        ownSt->setChecked(true);
        QDialog *dlgServer = new QDialog(this);
        dlgServer->setModal(true);
        dlgServer->setFixedSize(136,71); // Можно поменять
        dlgServer->setWindowTitle("Change server");
        QGridLayout *l = new QGridLayout(dlgServer);
        QLineEdit *leStatus = new QLineEdit(dlgServer);
        leStatus->setText(ownSt->text());
        l->addWidget(leStatus);
        QPushButton *accBut = new QPushButton("Accept",dlgServer);
        connect(accBut,SIGNAL(clicked()),dlgServer,SLOT(accept()));
        l->addWidget(accBut,1,0,1,2);
        dlgServer->setLayout(l);
        if(dlgServer->exec())
        {
            QRegExp expr("[^@!\"\\\\'&|\\*]+");
            QRegExp expr2("(online|gone|do not disturb)");
            if((expr.exactMatch(leStatus->text()))&&(!expr2.exactMatch(leStatus->text())))
            {
                QString newStatus=leStatus->text();
                if(newStatus.length()>19)
                    newStatus=newStatus.left(16)+"...";
                ownSt->setText(newStatus);
            }
        }
        _status=ownSt->text();
    }
    setWindowTitle(_status+' '+_sok->peerAddress().toString()+":"+QString::number(_sok->peerPort()));
}

//окно описания программы
void Dialog::aboutMe()
{
    QDialog *aboutMe = new QDialog(this);
    aboutMe->setModal(true);
    aboutMe->setWindowTitle("О программе");
    aboutMe->setMinimumSize(640,640);
    QPalette pal = aboutMe->palette();
    pal.setColor(aboutMe->backgroundRole(),Qt::black);
    aboutMe->setPalette(pal);
    QGridLayout *l = new QGridLayout(aboutMe);

    //добавление фото в описание
    QPixmap image;
    image.load("/Users/dmitriygarkin/Documents/photo.JPEG");
    QLabel *photoLb = new QLabel(aboutMe);
    photoLb->setPixmap(image.scaled(360,640,Qt::KeepAspectRatio));
    l->addWidget(photoLb,0,0,16,1,Qt::AlignCenter);

    //добавление тектового описания автора
    QLabel *meLb = new QLabel("Автор: Гаркин Дмитрий Александрович, СКБ211",aboutMe);
    l->addWidget(meLb,4,1);

    //добавление текстового описния даты и времени сборки
    QLabel *dateLb = new QLabel((QString)"Дата сборки: "+__DATE__+' '+__TIME__,aboutMe);
    l->addWidget(dateLb,5,1);

    //добавление текстового описания версии QT при сборке
    QLabel *versionOfBuild = new QLabel((QString)"Версия QT сборки: "+QT_VERSION_STR,aboutMe);
    l->addWidget(versionOfBuild,6,1);

    //добавление текстового описания версии QT при запуске
    QLabel *versionOfQt = new QLabel((QString)"Версия QT запущенная: "+qVersion(),aboutMe);
    l->addWidget(versionOfQt,7,1);

    //добавление кнопки для закрытия окна
    QPushButton *acc = new QPushButton("Закрыть",aboutMe);
    connect(acc,SIGNAL(clicked()),aboutMe,SLOT(accept()));

    l->addWidget(acc,15,1);
    aboutMe->setLayout(l);
    aboutMe->exec();
}

void Dialog::saveToXML()
{
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
    QDomElement root = document.createElement("Messages");
    //add it to document
    document.appendChild(root);

    for (int i = 0; i < messages.count(); ++i) {
        Message msg = messages[i];
        QDomElement log = document.createElement("Message");
        log.setAttribute("ip", msg.ip);
        log.setAttribute("time", msg.time);
        log.setAttribute("name", msg.name);
        log.setAttribute("type", msg.type);
        if (msg.type == comPlainText || msg.type == comRichText)
            log.setAttribute("content", msg.content.toString());
        else if (msg.type == comImage) {
            QImage img = msg.content.value<QImage>().scaled(320, 240);
            QSize size = img.size();
            int w = size.width(), h = size.height();
            QString content = QString::number(w) + ' ' + QString::number(h) + ' ';

            for (int i = 0; i < h; ++i)
                for (int j = 0; j < w; ++j)
                    content += img.pixelColor(j, i).name() + ' ';

            log.setAttribute("content", content);
        }
        else if (msg.type == 4)
            log.setAttribute("content", msg.content.toString());
        root.appendChild(log);
    }

    xmlContent << document.toString();
}

void Dialog::writeSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "HSE", "Lab6");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("font", ui->pteMessage->font().toString());
    settings.setValue("background", palette().color(backgroundRole()).name());
    settings.setValue("namecolor", currentNameColor.name());
    settings.setValue("statusbar", shownStatusBar);

    std::ofstream out;
    out.open(windowTitle().toStdString() + ".logs");
}

void Dialog::readSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "HSE", "Lab6");
    const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
    if (geometry.isEmpty()) {
        const QRect availableGeometry = screen()->availableGeometry();
        resize(availableGeometry.width() / 3, availableGeometry.height() / 2);
        move((availableGeometry.width() - width()) / 2,
             (availableGeometry.height() - height()) / 2);
    }
    else {
        restoreGeometry(geometry);
    }


    ui->pteMessage->setFont(settings.value("font").toString());


    QPalette p =ui->pteMessage->palette();

    p.setColor(backgroundRole(), settings.value("background").toString());
    setPalette(p);

    currentBackgroundColor = ui->pteMessage->palette().color(QPalette::Active, QPalette::Base);

    currentNameColor.setNamedColor(settings.value("namecolor").toString());

    shownStatusBar = settings.value("statusbar").toBool();
    SBD->setChecked(shownStatusBar);
}

void Dialog::nameColorDialog()
{
    const QColor color = QColorDialog::getColor(Qt::green, this, "Select Color");
    currentNameColor = color;
    updateMessages();
}

void Dialog::textColorDialog()
{
    const QColor color = QColorDialog::getColor(Qt::green, this, "Select Color");
    currentTextColor = color;
    updateMessages();
}

void Dialog::StatusBarDispaly(){
    if(SBD->isChecked()){
        statusBar()->show();
        shownStatusBar=1;
    }
    else{
        statusBar()->hide();
        shownStatusBar=0;
    }
}

void Dialog::ChangeBackgroundColor(){

    const QColor color = QColorDialog::getColor(Qt::green, this, "Select Color");
    QPalette p = ui->pteMessage->palette();
    currentBackgroundColor = color;
    //если надо у окон поставить фон
    //p.setColor(QPalette::Active, QPalette::Base, color.lighter(80));
    //p.setColor(QPalette::Inactive, QPalette::Base, color.lighter(80));
    p.setColor(backgroundRole(), color.lighter(80));
    setPalette(p);

}

void Dialog::MessageCustomization(){
    QDialog *mc = new QDialog(this);
    mc->setModal(true);
    mc->setWindowTitle("Кастомизация сообщений");
    mc->setMinimumSize(240,240);

    QPalette pal = mc->palette();
    pal.setColor(mc->backgroundRole(),Qt::white);
    mc->setPalette(pal);

    QVBoxLayout *l = new QVBoxLayout(mc);

    QPushButton * namecolor = new QPushButton("Names color",this);
    namecolor->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Maximum);
    connect(namecolor,SIGNAL(clicked()),this,SLOT(nameColorDialog()));

    QPushButton * textcolor = new QPushButton("Text color",this);
    textcolor->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Maximum);
    connect(textcolor,SIGNAL(clicked()),this,SLOT(textColorDialog()));

    QPushButton * exit = new QPushButton("Exit",mc);
    exit->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Maximum);
    connect(exit,SIGNAL(clicked()),mc,SLOT(accept()));

    l->addWidget(namecolor);
    l->addWidget(textcolor);
    l->addWidget(exit);
    mc->setLayout(l);
    mc->exec();
}

void Dialog::updateMessages() {
    ui->lwLog->clear();

    for (auto& msg : messages) {
        QFont f("Calibri");
        if (msg.type == comPlainText) {
            QString res;

            res = msg.content.toString();
            AddToLogRichText(res, currentTextColor, f);
            res = "";

            if (TD->isChecked())
                res += msg.time;
            if (IPAD->isChecked())
                res += msg.ip;

            res += "["+msg.name+"]: ";
            AddToLogRichText(res, currentNameColor, f);
        }
        else if (msg.type == comRichText)
        {
            QString res;

            auto tmp = msg.content.toString().split("\n");
            tmp.pop_front();
            tmp.pop_front();
            res = tmp.join("\n");
            if(currentTextColor.isValid())
                AddToLogRichText(res, currentTextColor, QFont(msg.content.toString().split("\n").at(1)));
            else
                AddToLogRichText(res, QColor(msg.content.toString().split("\n").at(0)), QFont(msg.content.toString().split("\n").at(1)));
            res = "";

            if (TD->isChecked())
                res += msg.time;
            if (IPAD->isChecked())
                res += msg.ip;

            res += "["+msg.name+"]: ";
            AddToLogRichText(res, currentNameColor, f);
        }
        else if (msg.type == comImage)
        {
            AddToLogImage(msg.content.value<QImage>());
        }
        else if (msg.type == 4)
        {
            AddToLogRichText("Recieved file " + msg.content.toString().split(" ").at(0)
                                 + " from user " + msg.name, currentTextColor, f);
        }
    }

}

void Dialog::chooseImage()
{
    ui->pbSend->setText("Send image");
    sendAction->setVisible(true);
    sendImageAction->setVisible(false);
    sendFTextAction->setVisible(true);
    QFileDialog openDialog(this, tr("Open image..."));
    openDialog.setAcceptMode(QFileDialog::AcceptOpen);
    openDialog.setFileMode(QFileDialog::ExistingFile);
    openDialog.setMimeTypeFilters({"image/jpeg", "image/png"});
    if(openDialog.exec() != QDialog::Accepted)
        return;
    QString fn = openDialog.selectedFiles().constFirst();
    image.load(fn);
    sendingImage = true;
    sendMsg();
    sendingImage = false;
}

void Dialog::chooseFile() {
    QFileDialog openDialog(this, tr("Choose File..."));
    openDialog.setAcceptMode(QFileDialog::AcceptOpen);
    openDialog.setFileMode(QFileDialog::ExistingFile);
    if(openDialog.exec() != QDialog::Accepted)
        return;
    QString fn = openDialog.selectedFiles().constFirst();

    QFile* m_file = new QFile(fn);
    if (!m_file->open(QIODevice::ReadOnly))
        return;

    chosenFileName = m_file->fileName();
    chosenFile = m_file->readAll();
    sendingFile = true;
    setProgressBar(0);
    sendMsg();
    sendingFile = false;
}

void Dialog::chooseFont()
{
    QFontDialog *df = new QFontDialog(this);
    df->exec();
    ui->pteMessage->setFont(df->selectedFont());
}

void Dialog::chooseColor()
{
    fontColor = QColorDialog::getColor(Qt::magenta, this);
    if(fontColor.isValid())
    {
        ui->pteMessage->setDocument(new QTextDocument(ui->pteMessage->toPlainText(),this));
        QPalette pal;
        pal.setColor(QPalette::Text, fontColor);
        ui->pteMessage->setPalette(pal);
    }
}

void Dialog::chooseSender()
{
    if(ui->pbSend->text() == "Send image") {
        chooseImage();
        return;
    }
    else if (ui->pteMessage->toPlainText() == "")
        return;
    else if (ui->pbSend->text() == "Formated text") {
        sendingFormatedText = true;
        sendMsg();
        sendingFormatedText = false;
        return;
    }
    sendMsg();
}

void Dialog::formatText()
{
    ui->pbSend->setText("Formated text");
    sendAction->setVisible(true);
    sendImageAction->setVisible(true);
    sendFTextAction->setVisible(false);
    ui->pbFont->setVisible(true);
    ui->pbColour->setVisible(true);
}

void Dialog::ProvideContextMenu(const QPoint &pos)
{
    QPoint item = ui->lwLog->mapToGlobal(pos);
    QMenu submenu;
    submenu.addAction("View in full size");
    submenu.addAction("Save image");
    QAction* rightClickItem = submenu.exec(item);
    if (rightClickItem && rightClickItem->text().contains("View in full size") )
    {
        showFullImage(ui->lwLog->itemAt(pos));
    }
    if (rightClickItem && rightClickItem->text().contains("Save image") )
    {
        saveImage(ui->lwLog->itemAt(pos));
    }
}

void Dialog::showFullImage(QListWidgetItem *arg) {
    QIcon icon = arg->icon();
    QDialog *dialog = new QDialog(this);
    dialog->setModal(false);
    dialog->setWindowTitle("Full image");

    QGridLayout *l = new QGridLayout(dialog);

    auto size = icon.availableSizes()[0];

    QLabel *photoLb = new QLabel(dialog);
    photoLb->setPixmap(icon.pixmap(size));
    l->addWidget(photoLb, 0, 0, Qt::AlignCenter);

    QPushButton *acc = new QPushButton("Close",dialog);
    connect(acc,SIGNAL(clicked()),dialog,SLOT(accept()));
    l->addWidget(acc, 1, 0);
    dialog->setLayout(l);
    dialog->exec();
}

void Dialog::saveImage(QListWidgetItem *arg) {
    QPixmap pixmap = arg->icon().pixmap(arg->icon().availableSizes()[0]);

    QFileDialog dialog(this);
    dialog.setWindowModality(Qt::WindowModal);
    dialog.setAcceptMode(QFileDialog::AcceptSave);
    if (dialog.exec() != QDialog::Accepted)
        return;

    auto fileName = dialog.selectedFiles().first();

    QFile file(fileName + ".png");
    file.open(QIODevice::WriteOnly);
    pixmap.save(&file, "PNG");
}

void Dialog::closeEvent(QCloseEvent *event)
{
    writeSettings();
    event->accept();
}

void Dialog::ProvideRightContextMenu(const QPoint &pos) {
    QPoint point = ui->lwUsers->mapToGlobal(pos);
    QMenu submenu;
    submenu.addAction("Information");
    submenu.addAction("Send file");
    QAction* rightClickItem = submenu.exec(point);
    //добавить проверку на попадание
    if (rightClickItem && rightClickItem->text().contains("Information") )
    {
        QListWidgetItem* item = ui->lwUsers->itemAt(pos);
        if (item == nullptr)
            return;
        askForInformation(item->text());
    }
    if (rightClickItem && rightClickItem->text().contains("Send file") )
    {
        QListWidgetItem* item = ui->lwUsers->itemAt(pos);
        if (item == nullptr)
            return;
        chosenUser = item->text();

        dialog = new QDialog(this);
        dialog->setWindowTitle("Send file");

        QGridLayout *l = new QGridLayout(dialog);

        QPushButton *file = new QPushButton("Choose file",dialog);
        connect(file,SIGNAL(clicked()), this, SLOT(chooseFile()));
        l->addWidget(file, 0, 0, Qt::AlignCenter);

        pb = new QProgressBar(dialog);
        pb->setMaximum(100);
        pb->setMinimum(0);
        pb->setValue(0);
        l->addWidget(pb, 1, 0, Qt::AlignCenter);

        QPushButton *acc = new QPushButton("Close",dialog);
        connect(acc,SIGNAL(clicked()),dialog,SLOT(accept()));
        l->addWidget(acc, 2, 0);
        dialog->setLayout(l);
        dialog->show();
    }
}

void Dialog::setUpSend()
{
    ui->pbSend->setText("Send");
    sendAction->setVisible(false);
    sendImageAction->setVisible(true);
    sendFTextAction->setVisible(true);
    ui->pbFont->setVisible(false);
    ui->pbColour->setVisible(false);
}

void Dialog::setProgressBar(int share) {
    pb->setValue(share);
}
