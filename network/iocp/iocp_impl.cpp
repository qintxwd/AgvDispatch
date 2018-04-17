
#include "iocp_impl.h"
#include "common_impl.h"
#include "tcpaccept_impl.h"
#include "tcpsocket_impl.h"
using namespace qyhnetwork;

#ifdef WIN32

bool EventLoop::initialize()
{
    if (_io != NULL)
    {
        LCF("iocp already craeted !  " << logSection());
        return false;
    }
    _io = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
    if (!_io)
    {
        LCF("CreateIoCompletionPort false!  " );
        return false;
    }
    return true;
}

void EventLoop::post(_OnPostHandler &&h)
{
    _OnPostHandler *ptr = new _OnPostHandler(std::move(h));
    PostQueuedCompletionStatus(_io, 0, PCK_USER_DATA, (LPOVERLAPPED)(ptr));
}

std::string EventLoop::logSection()
{
    std::stringstream os;
    os << "logSection[0x" << this << "]: _iocp=" << (void*)_io << ", current total timer=" << _timer.getTimersCount() <<", next expire time=" << _timer.getNextExpireTime();
    return os.str();
}



void EventLoop::runOnce(bool isImmediately)
{
    if (_io == NULL)
    {
        LCF("iocp handle not initialize. " <<logSection());
        return;
    }

    DWORD dwTranceBytes = 0;
    ULONG_PTR uComKey = NULL;
    LPOVERLAPPED pOverlapped = NULL;

    //    CompletionPort：指定的IOCP，该值由CreateIoCompletionPort函数创建。
    //    lpnumberofbytes：一次完成后的I/O操作所传送数据的字节数。
    //    lpcompletionkey：当文件I/O操作完成后，用于存放与之关联的CK。
    //    lpoverlapped：为调用IOCP机制所引用的OVERLAPPED结构。
    //    dwmilliseconds：用于指定调用者等待CP的时间。
    BOOL bRet = GetQueuedCompletionStatus(_io, &dwTranceBytes, &uComKey, &pOverlapped, isImmediately ? 0 : _timer.getNextExpireTime()/*INFINITE*/);

    _timer.checkTimer();
    if (!bRet && !pOverlapped)
    {
        //TIMEOUT
        return;
    }
    
    //! user post
    if (uComKey == PCK_USER_DATA)
    {
        _OnPostHandler * func = (_OnPostHandler*) pOverlapped;
        try
        {
            (*func)();
        }
        catch (const std::exception & e)
        {
            LCW("when call [post] callback throw one runtime_error. err=" << e.what());
        }
        catch (...)
        {
            LCW("when call [post] callback throw one unknown exception.");
        }
        delete func;
        return;
    }
    
    //! net data
    ExtendHandle & req = *(HandlerFromOverlaped(pOverlapped));
    switch (req._type)
    {
    case ExtendHandle::HANDLE_ACCEPT:
    {
        if (req._tcpAccept)
        {
            req._tcpAccept->onIOCPMessage(bRet);
        }
    }
        break;
    case ExtendHandle::HANDLE_RECV:
    case ExtendHandle::HANDLE_SEND:
    case ExtendHandle::HANDLE_CONNECT:
    {
        if (req._tcpSocket)
        {
            req._tcpSocket->onIOCPMessage(bRet, dwTranceBytes, req._type);
        }
    }
        break;
    default:
        LCE("GetQueuedCompletionStatus undefined type=" << req._type << logSection());
    }
    
}


#endif
