#-------------------------------------------------
#
# Project created by QtCreator 2018-04-08T15:53:55
#
#-------------------------------------------------

QT       -= gui core

TARGET = AgvDispatch
CONFIG   += console
CONFIG   -= app_bundle
CONFIG += c++11
TEMPLATE = app


SOURCES += main.cpp \
    agv.cpp \
    mapmanager.cpp \
    taskmanager.cpp \
    sqlite3/CppSQLite3.cpp \
    agvmanager.cpp \
    Common.cpp \
    qyhtcpclient.cpp

HEADERS += \
    agv.h \
    agvstation.h \
    agvline.h \
    mapmanager.h \
    agvtask.h \
    agvtasknode.h \
    agvtasknodedothing.h \
    taskmanager.h \
    sqlite3/CppSQLite3.h \
    sqlite3/sqlite3.h \
    Common.h \
    threadpool.h \
    agvmanager.h \
    qyhtcpclient.h

unix{
#LIBS=-ldl
LIBS += /usr/local/lib/libopencv_core.so
LIBS += /usr/local/lib/libopencv_highgui.so
LIBS += /usr/local/lib/libopencv_imgproc.so
LIBS += /usr/local/lib/libopencv_imgcodecs.so
LIBS += /usr/lib/x86_64-linux-gnu/libsqlite3.so
}

win32{
win32:DEFINES += _CRT_SECURE_NO_WARNINGS
INCLUDEPATH+=D:\thirdparty\sqlite\include
INCLUDEPATH+=D:\thirdparty\opencv3.3\include
INCLUDEPATH+=D:\thirdparty\opencv3.3\include\opencv
INCLUDEPATH+=D:\thirdparty\opencv3.3\include\opencv2
LIBS += D:\thirdparty\sqlite\lib\sqlite3.lib
LIBS += D:\thirdparty\opencv3.3\lib\opencv_highgui.lib
LIBS += D:\thirdparty\opencv3.3\lib\opencv_imgproc.lib
LIBS += D:\thirdparty\opencv3.3\lib\opencv_core.lib
LIBS += D:\thirdparty\opencv3.3\lib\opencv_imgcodecs.lib
}


