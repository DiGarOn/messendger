#ifndef MYCLIENT_H
#define MYCLIENT_H

#include <QObject>
#include <QDebug>
#include <QTcpSocket>
#include <QThreadPool>
#include <QtGui>
#include <QRegExp>
#include "myserver.h"

class MyServer;

class MyClient : public QObject
{
    //откроем доступ классу MyServer к приватному члену _sok
    friend class MyServer;
    Q_OBJECT

public:
    static const QString constNameUnknown;

    static const quint8 comPlainText = 1;
    static const quint8 comRichText = 2;
    static const quint8 comImage = 3;

    static const quint8 comAutchReq = 1;
    static const quint8 comUsersOnline = 2;
    static const quint8 comUserJoin = 3;
    static const quint8 comUserLeft = 4;
    static const quint8 comMessageToAll = 5;
    static const quint8 comMessageToUsers = 6;
    static const quint8 comPublicServerMessage = 7;
    static const quint8 comPrivateServerMessage = 8;
    static const quint8 comAutchSuccess = 9;
    static const quint8 comAskForInformation = 10;
    static const quint8 comGetStatus = 11;
    static const quint8 comFile = 12;
    static const quint8 comFileProgress = 13;

    static const quint8 comErrNameInvalid = 201;
    static const quint8 comErrNameUsed = 202;

    explicit MyClient(int desc, MyServer *serv, QObject *parent = 0);
    ~MyClient();
    void setName(QString name) {_name = name;}
    QString getName() const {return _name;}
    QPixmap getAvatar() const {return _avatar;}
    bool getAutched() const {return _isAutched;}
    void doSendCommand(quint8 comm) const;
    void doSendUsersOnline() const;

signals:
    //сигналы для обновления гуи
    void addUserToGui(QString name);
    void removeUserFromGui(QString name);
    void messageToGui(QString message, QString from, const QStringList &users);
    //сигнал удаления пользователя из QList
    void removeUser(MyClient *client);
//знакомые слоты для работы с сокетом
private slots:
    void onConnect();
    void onDisconnect();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError) const;
    void getStatus();

private:
    QTcpSocket *_sok;
    QHostAddress ip;
    MyServer *_serv;
    quint64 _blockSize;
    QString _name;
    QString connectedDate;
    QString _status;
    QPixmap _avatar;
    bool _isAutched;
};

#endif // MYCLIENT_H
