#-------------------------------------------------
#
# Project created by QtCreator 2011-10-13T22:35:09
#
#-------------------------------------------------

QT       += core gui network \
            xml \
            multimedia \
            widgets


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Lab2_client
TEMPLATE = app


SOURCES += main.cpp \
    dialog.cpp \


HEADERS  += dialog.h \
    message.hpp \


FORMS    += mainwindow.ui



