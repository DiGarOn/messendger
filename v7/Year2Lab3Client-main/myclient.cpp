#include "myclient.h"
#include <QWidget>
#include <QMessageBox>

const QString MyClient::constNameUnknown = QString(".Unknown");

MyClient::MyClient(int desc, MyServer *serv, QObject *parent) :QObject(parent)
{
    connectedDate = QDateTime::currentDateTime().toString("hh:mm:ss");

    _serv = serv;

    _isAutched = false;
    _name = constNameUnknown;

    _blockSize = 0;

    _sok = new QTcpSocket(this);

    _sok->setSocketDescriptor(desc);

    connect(_sok, SIGNAL(connected()), this, SLOT(onConnect()));
    connect(_sok, SIGNAL(disconnected()), this, SLOT(onDisconnect()));
    connect(_sok, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(_sok, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));

    ip = _sok->peerAddress();

    qDebug() << "Client connected" << desc;
}

MyClient::~MyClient()
{

}

void MyClient::onConnect()
{

    //never calls, socket already connected to the tcpserver
    //we just binding to this socket here: _sok->setSocketDescriptor(desc);
}

void MyClient::onDisconnect()
{
    qDebug() << "Client disconnected";

    if (_isAutched)
    {

        emit removeUserFromGui(_name);

        _serv->doSendToAllUserLeft(_name);


        emit removeUser(this);
    }
    deleteLater();
}

void MyClient::onError(QAbstractSocket::SocketError socketError) const
{

    QWidget w;
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(&w, "Error", "The host was not found");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(&w, "Error", "The connection was refused by the peer.");
        break;
    default:
        QMessageBox::information(&w, "Error", "The following error occurred: "+_sok->errorString());
    }

}

void MyClient::onReadyRead()
{
    QDataStream in(_sok);

    if (_blockSize == 0) {

        if (_sok->bytesAvailable() < (int)sizeof(quint64))
            return;

        in >> _blockSize;
        qDebug() << "_blockSize now " << _blockSize;
    }


    if ((quint64)_sok->bytesAvailable() < _blockSize)
        return;
    else
        _blockSize = 0;

    quint8 command;
    in >> command;
    qDebug() << "Received command " << command;

    if (!_isAutched && command != comAutchReq)
        return;

    switch(command)
    {

        case comAutchReq:
        {
            QString name;
            QPixmap avatar;
            in >> name;
            in >> avatar;

            if (!_serv->isNameValid(name))
            {
                doSendCommand(comErrNameInvalid);
                return;
            }

            if (_serv->isNameUsed(name))
            {
                doSendCommand(comErrNameUsed);
                return;
            }

            _name = name;
            _avatar = avatar;
            _isAutched = true;

            doSendUsersOnline();

            emit addUserToGui(name);

            _serv->doSendToAllUserJoin(_name, _avatar);
        }
        break;

        case comMessageToAll:
        {
            QString time;
            in >> time;
            quint8 type;
            in >> type;
            if (type == comPlainText) {
                QString message;
                in >> message;
                _serv->doSendToAllMessage(ip.toString() + ' ', time, type, message, _name);

                //разобраться с сигналом
                emit messageToGui(message, _name, QStringList());
            }
            else if (type == comRichText) {
                QString colorName, fontKey, message;
                in >> colorName >> fontKey >> message;
                _serv->doSendToAllMessage(ip.toString() + ' ', time, type, colorName, fontKey, message, _name);


            }
            else if (type == comImage) {
                QImage image;
                in >> image;
                _serv->doSendToAllMessage(ip.toString() + ' ', time, type, image, _name);
            }
        }
        break;

        case comMessageToUsers:
        {
            QString users_in;
            in >> users_in;
            QString message;
            in >> message;

            QStringList users = users_in.split(",");

            _serv->doSendMessageToUsers(message, users, _name);

            emit messageToGui(message, _name, users);
        }
        break;

        case comAskForInformation:
        {
            QString from;
            in >> from;
            QString about;
            in >> about;
            _serv->getInformation(from, about);
        }
        break;

        case comGetStatus:
        {
            QString status;
            in >> status;
            _status = status;
            _serv->writeInformation();
        }
        break;

        case comFile: {
            QString time;
            in >> time;


            QString userName;
            in >> userName;

            QString fileName;
            // get sending file name
            in >> fileName;

            QString userFromName;
            in >> userFromName;

            QByteArray line = _sok->readAll();

            QByteArray block;
            QDataStream out(&block, QIODevice::WriteOnly);
            out << (quint64)0;
            out << MyClient::comFileProgress;

            out << 50;

            out.device()->seek(0);
            out << (quint64)(block.size() - sizeof(quint64));
            _sok->write(block);

            _serv->doSendFile(ip.toString() + ' ', time, userName, fileName, userFromName, line);

            /*QString filePath = ""; // your file path for receiving
            fileName = fileName.section("/", -1);
            QFile target(filePath + '/' + fileName);
            if (!target.open(QIODevice::WriteOnly)) {
                qDebug() << "Can't open file for written";
                return;
            }
            target.write(line);

            target.close();*/

        }
        break;

        case comFileProgress: {
            QString userName;
            in >> userName;
            int progress;
            in >> progress;

            _serv->sendProgressToUser(userName, progress);
        }
        break;
    }

    //for (long long i = 0; i < 4000000000; ++i){}
}

void MyClient::doSendCommand(quint8 comm) const
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint64)0;
    out << comm;
    out.device()->seek(0);
    out << (quint64)(block.size() - sizeof(quint64));
    _sok->write(block);
    qDebug() << "Send to" << _name << "command:" << comm;
}

void MyClient::doSendUsersOnline() const
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint64)0;
    out << comUsersOnline;

    QStringList l = _serv->getUsersOnline();
    QVector <QPixmap> av = _serv->getUsersAvatarsOnline();

    out << l.length() - 1;

    for (int i = 0; i < l.length(); ++i)
        if (l.at(i) != _name)
            out << l.at(i) << av.at(i);

    out.device()->seek(0);
    out << (quint64)(block.size() - sizeof(quint64));
    _sok->write(block);
}

void MyClient::getStatus() {
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint64)0;
    out << comGetStatus;
    out.device()->seek(0);
    out << (quint64)(block.size() - sizeof(quint64));
    _sok->write(block);
    qDebug() << "Getting status";
}

