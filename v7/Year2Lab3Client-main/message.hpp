#ifndef MESSAGE_HPP
#define MESSAGE_HPP
#include <QString>
#include <QVariant>

struct Message
{
    Message(QString ip1, QString time1, QString name1, quint8 type1, QVariant content1) {
        ip = ip1;
        time = time1;
        name = name1;
        type = type1;
        content = content1;
    }

    QString ip;
    QString time;
    QString name;
    quint8 type;
    QVariant content;
};

#endif // MESSAGE_HPP
