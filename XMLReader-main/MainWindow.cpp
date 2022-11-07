#include "MainWindow.h"
#include <QMenuBar>
#include <QApplication>
#include <QFileDialog>
#include <QXmlStreamReader>
#include <QtDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setMinimumSize(640,480);
    setWindowTitle("Open file in menu File");
    QMenu* menu=menuBar()->addMenu("&File");
    menu->addAction("Open",this,SLOT(openSlot()));
    menu->addAction("Close",this,SLOT(closeSlot()));
    menu->addAction("Exit",QApplication::instance(),SLOT(quit()));
    lwLogs=new QListWidget(this);
    lwLogs->setFrameStyle(0);
    lwLogs->setIconSize(QSize(320, 240));
    setCentralWidget(lwLogs);
}

void MainWindow::openSlot()
{
    QString filename = QFileDialog::getOpenFileName(this,"Choose menu",QDir::homePath(),"XML (*.xml)");
    if(filename=="") return;
    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) return;
    QXmlStreamReader reader(&file);
    reader.readNextStartElement();
    if(reader.name()!="Messages") return;
    closeSlot();
    setWindowTitle("Openned: "+filename.split('/').back());
    while(!reader.atEnd())
    {
        if(reader.isStartElement())
        {
            if(reader.name()=="Message")
            {
                QString time,nickname,ip,content,typeStr;
                foreach(const QXmlStreamAttribute &attr, reader.attributes())
                {
                    if(attr.name().toString()=="time")
                        time=attr.value().toString();
                    if(attr.name().toString()=="name")
                        nickname=attr.value().toString();
                    if(attr.name().toString()=="ip")
                        ip=attr.value().toString();
                    if(attr.name().toString()=="content")
                        content=attr.value().toString();
                    if(attr.name().toString()=="type")
                        typeStr=attr.value().toString();
                }
                int type = typeStr.toInt();
                if(type==1)
                {
                         lwLogs->insertItem(0,content);
                         lwLogs->insertItem(0,time+' '+ip+' '+nickname+" : ");
                }
                if(type==2)
                {
                    QStringList data2=content.split('\n');
                    QColor clr(data2.takeFirst());
                    QFont font(data2.takeFirst());
                    lwLogs->insertItem(0,data2.join('\n'));
                    lwLogs->item(0)->setForeground(clr);
                    lwLogs->item(0)->setFont(font);
                    lwLogs->insertItem(0,time+' '+ip+' '+nickname+" : ");
                }
                if(type==3)
                {
                    QStringList data = content.split(' ');
                    int width=data.takeFirst().toInt();
                    int height=data.takeFirst().toInt();
                    QImage img(width,height,QImage::Format::Format_RGB32);
                    for(int i=0;i<height;++i)
                        for(int j=0;j<width;++j)
                            img.setPixelColor(j,i,data.takeFirst());
                    QListWidgetItem *item= new QListWidgetItem(QIcon(QPixmap::fromImage(img)),"");
                    lwLogs->insertItem(0,item);
                    lwLogs->insertItem(0,time+' '+ip+' '+nickname+" : ");
                }
                if(type==4)
                {
                    QStringList data = content.split(' ');
                    QString filename = data.takeFirst();
                    lwLogs->insertItem(0,"Hash: "+data.join(' '));
                    lwLogs->insertItem(0,"File: "+filename);
                    lwLogs->insertItem(0,time+' '+ip+' '+nickname+" : ");
                }
            }
        }
        reader.readNext();
    }
}

void MainWindow::closeSlot()
{
    lwLogs->clear();
    setWindowTitle("Open file in menu File");
}
