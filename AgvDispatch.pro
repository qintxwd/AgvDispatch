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
    qyhtcpclient.cpp \
    network/epoll/epoll_impl.cpp \
    network/epoll/tcpaccept_impl.cpp \
    network/epoll/tcpsocket_impl.cpp \
    network/epoll/udpsocket_impl.cpp \
    network/timer/timer.cpp \
    network/common/networkcommon.cpp \
    network/epoll/epoll_impl.cpp \
    network/epoll/tcpaccept_impl.cpp \
    network/epoll/tcpsocket_impl.cpp \
    network/epoll/udpsocket_impl.cpp \
    network/tcpsession.cpp \
    network/sessionmanager.cpp \
    network/iocp/iocp_impl.cpp \
    network/iocp/tcpaccept_impl.cpp \
    network/iocp/tcpsocket_impl.cpp \
    network/iocp/udpsocket_impl.cpp

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
    qyhtcpclient.h \
    network/epoll/common_impl.h \
    network/epoll/epoll_impl.h \
    network/epoll/tcpaccept_impl.h \
    network/epoll/tcpsocket_impl.h \
    network/epoll/udpsocket_impl.h \
    network/timer/timer.h \
    network/common/networkcommon.h \
    network/epoll/common_impl.h \
    network/epoll/epoll_impl.h \
    network/epoll/tcpaccept_impl.h \
    network/epoll/tcpsocket_impl.h \
    network/epoll/udpsocket_impl.h \
    network/networkconfig.h \
    network/tcpsession.h \
    network/sessionmanager.h \
    network/iocp/common_impl.h \
    network/iocp/iocp_impl.h \
    network/iocp/tcpaccept_impl.h \
    network/iocp/tcpsocket_impl.h \
    network/iocp/udpsocket_impl.h

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
INCLUDEPATH+=D:\thirdparty\opencv\opencv3.3\include
LIBS+=D:\thirdparty\sqlite\lib\Debug\sqlite3.lib
LIBS+=D:\thirdparty\opencv\opencv3.3\lib\opencv_world330d.lib
}


