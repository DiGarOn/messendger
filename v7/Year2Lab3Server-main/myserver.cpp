#include "myserver.h"
#include "QWidget"

MyServer::MyServer(QWidget *widget, QObject *parent) :QTcpServer(parent)
{
    _widget = widget;
}

bool MyServer::doStartServer(QHostAddress addr, qint16 port)
{
    if (!listen(addr, port))
    {
        qDebug() << "Server not started at" << addr << ":" << port;
        return false;
    }
    qDebug() << "Server started at" << addr << ":" << port;
    return true;
}

void MyServer::doSendToAllUserJoin(QString name, QPixmap avatar)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);

    out << (quint64)0 << MyClient::comUserJoin << name << avatar.toImage();

    out.device()->seek(0);
    out << (quint64)(block.size() - sizeof(quint64));

    for (int i = 0; i < _clients.length(); ++i)
        if (_clients.at(i)->getName() != name && _clients.at(i)->getAutched())
            _clients.at(i)->_sok->write(block);
}

void MyServer::doSendToAllUserLeft(QString name)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint64)0 << MyClient::comUserLeft << name;
    out.device()->seek(0);
    out << (quint64)(block.size() - sizeof(quint64));
    for (int i = 0; i < _clients.length(); ++i)
        if (_clients.at(i)->getName() != name && _clients.at(i)->getAutched())
            _clients.at(i)->_sok->write(block);
}

void MyServer::doSendToAllMessage(QString ip, QString time, quint8 type, QVariant content, QString fromUsername)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    if (type == MyClient::comPlainText)
        out << (quint64)0 << MyClient::comMessageToAll << fromUsername << ip << time << type << content.toString();
    else if (type == MyClient::comImage)
        out << (quint64)0 << MyClient::comMessageToAll << fromUsername << ip << time << type << content.value<QImage>();

    out.device()->seek(0);
    out << (quint64)(block.size() - sizeof(quint64));
    for (int i = 0; i < _clients.length(); ++i)
        if (_clients.at(i)->getAutched())
            _clients.at(i)->_sok->write(block);
}

void MyServer::doSendToAllMessage(QString ip, QString time, quint8 type, QString colorName, QString fontKey, QString message, QString fromUsername) {
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    if (type == MyClient::comRichText)
        out << (quint64)0 << MyClient::comMessageToAll << fromUsername << ip << time << type << colorName
            << fontKey << message;

    out.device()->seek(0);
    out << (quint64)(block.size() - sizeof(quint64));
    for (int i = 0; i < _clients.length(); ++i)
        if (_clients.at(i)->getAutched())
            _clients.at(i)->_sok->write(block);
}

void MyServer::doSendToAllServerMessage(QString message)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint64)0 << MyClient::comPublicServerMessage << message;
    out.device()->seek(0);
    out << (quint64)(block.size() - sizeof(quint64));
    for (int i = 0; i < _clients.length(); ++i)
        if (_clients.at(i)->getAutched())
            _clients.at(i)->_sok->write(block);
}

void MyServer::doSendServerMessageToUsers(QString message, const QStringList &users)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint64)0 << MyClient::comPrivateServerMessage << message;
    out.device()->seek(0);
    out << (quint64)(block.size() - sizeof(quint64));
    for (int j = 0; j < _clients.length(); ++j)
        if (users.contains(_clients.at(j)->getName()))
            _clients.at(j)->_sok->write(block);
}
/*
При рассылке сообщения всем нужно делать проверку авторизован ли текущий пользователь, ибо в массиве _clients, возможно, находятся не авторизованные
*/
void MyServer::doSendMessageToUsers(QString message, const QStringList &users, QString fromUsername)
{
    //знакомые по клиенту действия
    QByteArray block, blockToSender;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint64)0 << MyClient::comMessageToUsers << fromUsername << message;
    out.device()->seek(0);
    out << (quint64)(block.size() - sizeof(quint64));


    QDataStream outToSender(&blockToSender, QIODevice::WriteOnly);
    outToSender << (quint64)0 << MyClient::comMessageToUsers << users.join(",") << message;
    outToSender.device()->seek(0);
    outToSender << (quint64)(blockToSender.size() - sizeof(quint64));
    //отправка сообщения всем (тут отсутствует проверка, ибо все пользователи в users гарантированно авторизованы)
    for (int j = 0; j < _clients.length(); ++j)
        if (users.contains(_clients.at(j)->getName()))
            _clients.at(j)->_sok->write(block);
        else if (_clients.at(j)->getName() == fromUsername)
            _clients.at(j)->_sok->write(blockToSender);
}

void MyServer::doSendFile(QString ip, QString time, QString toUser, QString fileName, QString userFromName, QByteArray file) {
    int sendTo = -1;

    for (int j = 0; j < _clients.length(); ++j)
        if (_clients.at(j)->getName() == toUser) {
            sendTo = j;
            break;
        }

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint64)0;
    out << MyClient::comFile;

    out << ip;
    out << time;

    out << fileName;
    out << userFromName;
    block.append(file);

    out.device()->seek(0);
    out << (quint64)(block.size() - sizeof(quint64));

    qint64 x = 0;
    while (x < block.size()) {
        qint64 y = _clients.at(sendTo)->_sok->write(block);
        x += y;

        //qDebug() << x;    // summary size you send, so you can check recieved and replied sizes
    }

}

void MyServer::getInformation(QString from, QString about) {

    for (int j = 0; j < _clients.length(); ++j)
        if (_clients.at(j)->getName() == from)
            asking = j;
        else if (_clients.at(j)->getName() == about)
            answering = j;

    _clients.at(answering)->getStatus();//writes value in field _status
    qDebug() << "Getting information";
}

void MyServer::writeInformation() {
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint64)0 << MyClient::comAskForInformation << _clients.at(answering)->ip.toString();
    out << _clients.at(answering)->connectedDate << _clients.at(answering)->_status;
    out.device()->seek(0);
    out << (quint64)(block.size() - sizeof(quint64));
    _clients.at(asking)->_sok->write(block);
}

QStringList MyServer::getUsersOnline() const
{
    QStringList l;
    foreach (MyClient * c, _clients)
        if (c->getAutched())
            l << c->getName();
    return l;
}

QVector <QPixmap> MyServer::getUsersAvatarsOnline() const {
    QVector <QPixmap> av;
    foreach (MyClient * c, _clients)
        if (c->getAutched())
            av.push_back(c->_avatar);
    return av;
}

bool MyServer::isNameValid(QString name) const
{
    if (name.length() > 20 || name.length() < 5)
        return false;
    QRegExp r("[A-Za-z0-9_]+");
    return r.exactMatch(name);
}

bool MyServer::isNameUsed(QString name) const
{
    for (int i = 0; i < _clients.length(); ++i)
        if (_clients.at(i)->getName() == name)
            return true;
    return false;
}

void MyServer::incomingConnection(qintptr handle)
{

    //передаем дескрпитор сокета, указатель на сервер (для вызова его методов), и стандартный параметр - parent
    MyClient *client = new MyClient(handle, this, this);
    //подключаем сигналы напрямую к виджету, если его передали в конструктор сервера
   if (_widget != 0)
    {
        connect(client, SIGNAL(addUserToGui(QString)), _widget, SLOT(onAddUserToGui(QString)));
        connect(client, SIGNAL(removeUserFromGui(QString)), _widget, SLOT(onRemoveUserFromGui(QString)));
        connect(client, SIGNAL(messageToGui(QString,QString,QStringList)), _widget, SLOT(onMessageToGui(QString,QString,QStringList)));
    }
    connect(client, SIGNAL(removeUser(MyClient*)), this, SLOT(onRemoveUser(MyClient*)));
    _clients.append(client);
}

void MyServer::onRemoveUser(MyClient *client)
{
    _clients.removeAt(_clients.indexOf(client));
}

void MyServer::onMessageFromGui(QString message, const QStringList &users)
{
    if (users.isEmpty())
        doSendToAllServerMessage(message);
    else
        doSendServerMessageToUsers(message, users);
}

void MyServer::sendProgressToUser(QString userName, int progress) {
    int sendTo;

    for (int j = 0; j < _clients.length(); ++j)
        if (_clients.at(j)->getName() == userName)
            sendTo = j;

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint64)0;
    out << MyClient::comFileProgress;

    out << progress;

    out.device()->seek(0);
    out << (quint64)(block.size() - sizeof(quint64));
    _clients.at(sendTo)->_sok->write(block);
}
