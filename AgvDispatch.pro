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
win32:DEFINES += _WINSOCK_DEPRECATED_NO_WARNINGS
INCLUDEPATH+=D:\thirdparty\sqlite\include
INCLUDEPATH+=D:\thirdparty\opencv\opencv3.3\include
LIBS+=D:\thirdparty\sqlite\lib\Debug\sqlite3.lib
LIBS+=D:\thirdparty\opencv\opencv3.3\lib\opencv_world330d.lib
}

HEADERS += \
    agv.h \
    agvline.h \
    agvmanager.h \
    agvstation.h \
    agvtask.h \
    agvtasknode.h \
    agvtasknodedothing.h \
    Common.h \
    mapmanager.h \
    qyhtcpclient.h \
    taskmanager.h \
    threadpool.h \
    sqlite3/CppSQLite3.h \
    network/networkconfig.h \
    network/sessionmanager.h \
    network/tcpsession.h \
    network/timer/timer.h \
    network/common/networkcommon.h \
    network/epoll/epoll_common.h \
    network/epoll/epoll.h \
    network/epoll/epollaccept.h \
    network/epoll/epollsocket.h \
    network/iocp/iocp_common.h \
    network/iocp/iocp.h \
    network/iocp/iocpsocket.h \
    network/iocp/iocpaccept.h \
    utils/Log/easylogging.h \
    Protocol.h \
    utils/noncopyable.h \
    msgprocess.h

SOURCES += \
    agv.cpp \
    agvmanager.cpp \
    Common.cpp \
    main.cpp \
    mapmanager.cpp \
    qyhtcpclient.cpp \
    taskmanager.cpp \
    sqlite3/CppSQLite3.cpp \
    network/sessionmanager.cpp \
    network/tcpsession.cpp \
    network/timer/timer.cpp \
    network/common/networkcommon.cpp \
    network/epoll/epoll.cpp \
    network/epoll/epollaccept.cpp \
    network/epoll/epollsocket.cpp \
    network/iocp/iocp.cpp \
    network/iocp/iocpsocket.cpp \
    network/iocp/iocpaccept.cpp \
    utils/Log/easylogging.cpp \
    msgprocess.cpp


