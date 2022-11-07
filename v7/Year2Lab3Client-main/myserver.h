#ifndef MYSERVER_H
#define MYSERVER_H

#include <QTcpServer>
#include <QDebug>
#include "myclient.h"
class MyClient;

class MyServer : public QTcpServer
{
    Q_OBJECT

public:
    explicit MyServer(QWidget *widget = 0, QObject *parent = 0);
    bool doStartServer(QHostAddress addr, qint16 port);
    void doSendToAllUserJoin(QString name, QPixmap avatar);                         //уведомить о новом пользователе
    void doSendToAllUserLeft(QString name);
    void doSendToAllMessage(QString ip, QString time, quint8 type, QVariant content, QString fromUsername);//разослать сообщение
    void doSendToAllMessage(QString ip, QString time, quint8 type, QString colorName, QString fontKey, QString message, QString _name);
    void doSendToAllServerMessage(QString message);                                 //серверное сообщение
    void doSendServerMessageToUsers(QString message, const QStringList &users);     //приватное серверное сообщение
    void doSendMessageToUsers(QString message, const QStringList &users, QString fromUsername);
    void doSendFile(QString ip, QString time, QString toUser, QString fileName, QString usetFromName, QByteArray file);
    void sendProgressToUser(QString userName, int progress);
    void getInformation(QString from, QString about);
    QStringList getUsersOnline() const;
    QVector <QPixmap> getUsersAvatarsOnline() const;                                        //узнать список пользователей
    bool isNameValid(QString name) const;                                           //проверить имя
    bool isNameUsed(QString name) const;                                            //проверить используется ли имя

signals:
    void addLogToGui(QString string, QColor color = Qt::black);

public slots:
    void onMessageFromGui(QString message, const QStringList &users);
    void onRemoveUser(MyClient *client);
    void writeInformation();

protected:
    void incomingConnection(qintptr handle);

private:
    QList<MyClient *> _clients;
    QWidget *_widget;
    int asking;
    int answering;                                                      //ссылка на виджет для подключения к нему сигналов от myclient

};

#endif // MYSERVER_H
