#ifndef DIALOG_H
#define DIALOG_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QHostAddress>
#include "/Users/dmitriygarkin/Documents/PL/qt/LR6 2/v7/Year2Lab3Server-main/myclient.h" //изменить на свой путь
#include <QMessageBox>
#include <QLineEdit>
#include <QListWidgetItem>
#include "message.hpp"
#include <QShortcut>
#include <QMediaPlayer>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QProgressBar>
#include <QLabel>

class MyClient;

namespace Ui {
    class MainWindow;
}

class Dialog : public QMainWindow
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = 0);
    ~Dialog();
    static const quint8 comPlainText = 1;
    static const quint8 comRichText = 2;
    static const quint8 comImage = 3;

private slots:
    //определим слоты для обработки сигналов сокета
    void onSokConnected();
    void onSokDisconnected();
    //сигнал readyRead вызывается, когда сокет получает пакет (который может быть лишь частью отправленых данных) байтов
    void formatText();
    void onSokReadyRead();
    void onSokDisplayError(QAbstractSocket::SocketError socketError);
    void chooseFont();
    void chooseImage();
    void chooseColor();
    void chooseAvatar();
    void connectionButton();
    void disconnectButton();
    void changeServer();
    void autoFileAcceptSlot(bool b);
    void changeName();
    void changeStatus();
    void on_cbToAll_clicked();
    void sendMsg();
    void aboutMe();
    void saveToXML();
    void ChangeBackgroundColor();
    void MessageCustomization();
    void StatusBarDispaly();

    //настройка внешнего вида даилога
    void textColorDialog();
    void nameColorDialog();

    //изменение/чтение настроек
    void readSettings();
    void writeSettings();


    void updateMessages();
    void ProvideRightContextMenu(const QPoint &pos);
    void ProvideContextMenu(const QPoint &pos);
    void showFullImage(QListWidgetItem *);
    void saveImage(QListWidgetItem *arg);
    void setUpSend();
    void chooseFile();

    void chooseSender();

    void askForInformation(QString name);
    void setProgressBar(int share);

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;
    QTcpSocket *_sok;                       //наш сокет
    QHostAddress _ip;                       //адрес подключения
    quint16 _port;                          //порт подключения
    quint64 _blockSize;                     //текущий размер блока данных
    QString _name, _status;                 //имя клиента и его статус
    QAction *connectAct, *disconnectAct;    //подключение/отключение к/от серверу/а
    QAction *serverAct;
    QAction *userPhoto;                     //поле для хранения фото (аватара)
    QPixmap avatar;
    QPixmap image;                          //поле для хранения фото для отправки
    QAction *onlineSt, *goneSt, *doNotDisturbSt, *ownSt; //статусы
    QAction *sendImageAction, *sendFTextAction, *sendAction; //поля описания действий (варианты отправки)
    bool autoFileAcception = true;
    void AddToLog(QString text, QColor color = Qt::black);          //получение текстового сообщения
    void AddToLogImage(QImage img);                                 //получение картинки
    void AddToLogRichText(QString text, QColor color, QFont font);  //получение красивого сообщения

    QVector <Message> messages;
    QAction *CBC;                       //ChangeBackgroundColor
    QAction *MC;                        //MessageCustomization
    QAction *SBD;                       //StatusBarDisplay
    QAction *IPAD;                      //IPAdressDisplay
    QAction *TD;                        //TimeDisplay

    bool shownStatusBar = 0;
    QColor currentTextColor;
    QColor currentNameColor = Qt::black;
    QColor currentBackgroundColor = Qt::white;
    QColor fontColor;
    QPalette currentPalette;

    QVector<QString> selectedUserInformation;

    bool sendingImage = false;
    bool sendingFormatedText = false;
    bool sendingFile = false;

    QMediaPlayer newMessage;

    QString chosenUser;
    QString chosenFileName;
    QByteArray chosenFile;

    QDialog *dialog;
    QProgressBar *pb;
};

#endif // DIALOG_H
