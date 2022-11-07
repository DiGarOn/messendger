#-------------------------------------------------
#
# Project created by QtCreator 2011-10-13T18:08:44
#
#-------------------------------------------------

QT       += core gui network \
            xml
PKGCONFIG += openssl
LIBS += -lcrypto
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Lab2_server
TEMPLATE = app


SOURCES += main.cpp\
    dialog.cpp \
    mainwindow.cpp \
    myclient.cpp \
    myserver.cpp

HEADERS  += \
    mainwindow.hpp \
    myserver.h \
    dialog.h \
    myclient.h

FORMS    += dialog.ui














