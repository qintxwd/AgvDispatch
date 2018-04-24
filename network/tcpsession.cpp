#include "tcpsession.h"
#include "sessionmanager.h"
#include "../Common.h"
#include "../msgprocess.h"

using namespace qyhnetwork;
using std::min;
using std::max;

namespace qyhnetwork {


TcpSession::TcpSession():read_position(0),username("")
{
    SessionManager::getInstance()->_statInfo[STAT_SESSION_CREATED]++;
}

TcpSession::~TcpSession()
{
    SessionManager::getInstance()->_statInfo[STAT_SESSION_DESTROYED]++;
    if (_sockptr)
    {
        _sockptr->doClose();
        _sockptr.reset();
    }
}


void TcpSession::connect()
{
    if (_status == 0)
    {
        _pulseTimerID = SessionManager::getInstance()->createTimer(_options._connectPulseInterval, std::bind(&TcpSession::onPulse, shared_from_this()));
        _status = 1;
        reconnect();
    }
    else
    {
        LOG(ERROR)<<"can't connect on a old session.  please use addConnect try again.";
    }
}

void TcpSession::reconnect()
{
    if (_sockptr)
    {
        _sockptr->doClose();
        _sockptr.reset();
    }
    _sockptr = std::make_shared<TcpSocket>();
    if (!_sockptr->initialize(_eventLoop))
    {
        LOG(ERROR)<<"connect init error";
        return;
    }

    read_position = 0;

    if (!_sockptr->doConnect(_remoteIP, _remotePort, std::bind(&TcpSession::onConnected, shared_from_this(), std::placeholders::_1)))
    {
        LOG(ERROR)<<"connect error";
        return;
    }
}

bool TcpSession::attatch(const TcpSocketPtr &sockptr, AccepterID aID, SessionID sID)
{
    _sockptr = sockptr;
    _acceptID = aID;
    _sessionID = sID;
    _sockptr->getPeerInfo(_remoteIP, _remotePort);
    if (_options._setNoDelay)
    {
        sockptr->setNoDelay();
    }
    _status = 2;
    _pulseTimerID = SessionManager::getInstance()->createTimer(_options._sessionPulseInterval, std::bind(&TcpSession::onPulse, shared_from_this()));
    SessionManager::getInstance()->_statInfo[STAT_SESSION_LINKED]++;

    if (_options._onSessionLinked)
    {
        try
        {
            _options._onSessionLinked(shared_from_this());
        }
        catch (const std::exception & e)
        {
            LOG(ERROR)<<"TcpSession::attatch _onSessionLinked error. e=" << e.what();
        }
        catch (...)
        {
            LOG(WARNING)<<"TcpSession::attatch _onSessionLinked catch one unknown exception.";
        }
    }

    if (!doRecv())
    {
        close();
        return false;
    }
    return true;
}

void TcpSession::onConnected(qyhnetwork::NetErrorCode ec)
{
    if (ec)
    {
        LOG(WARNING)<<"onConnected error. ec=" << ec << ",  cID=" << _sessionID;
        return;
    }
    LOG(INFO)<<"onConnected success. sessionID=" << _sessionID;

    if (!doRecv())
    {
        return;
    }
    _status = 2;
    _reconnects = 0;
    if (_options._setNoDelay)
    {
        _sockptr->setNoDelay();
    }
    if (_options._onSessionLinked)
    {
        try
        {
            _options._onSessionLinked(shared_from_this());
        }
        catch (const std::exception & e)
        {
            LOG(ERROR)<<"TcpSession::onConnected error. e=" << e.what();
        }
        catch (...)
        {
            LOG(WARNING)<<"TcpSession::onConnected catch one unknown exception.";
        }
    }
    SessionManager::getInstance()->_statInfo[STAT_SESSION_LINKED]++;
}

bool TcpSession::doRecv()
{
    LOG(TRACE)<<"TcpSession::doRecv sessionID=" << getSessionID() ;
    if (!_sockptr)
    {
        return false;
    }
#ifndef WIN32
    return _sockptr->doRecv(read_buffer + read_position, ONE_MSG_MAX_LENGTH - read_position, std::bind(&TcpSession::onRecv, shared_from_this(), std::placeholders::_1, std::placeholders::_2), true);
#else
    return _sockptr->doRecv(read_buffer + read_position, ONE_MSG_MAX_LENGTH - read_position, std::bind(&TcpSession::onRecv, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
#endif
}

void TcpSession::close()
{
    if (_status != 3 && _status != 0)
    {
        if (_sockptr)
        {
            _sockptr->doClose();
            _sockptr.reset();
        }
        LOG(INFO)<<"TcpSession to close socket. sID= " << _sessionID;
        if (_status == 2)
        {
            SessionManager::getInstance()->_statInfo[STAT_SESSION_CLOSED]++;
            if (_options._onSessionClosed)
            {
                SessionManager::getInstance()->post(std::bind(_options._onSessionClosed, shared_from_this()));
            }
        }

        if (isConnectID(_sessionID) && _reconnects < _options._reconnects)
        {
            _status = 1;
            LOG(INFO)<<"TcpSession already closed. try reconnect ... sID= " << _sessionID;
        }
        else
        {
            _status = 3;
            SessionManager::getInstance()->post(std::bind(&SessionManager::removeSession, SessionManager::getInstance(), shared_from_this()));
            LOG(INFO)<<"TcpSession remove self from manager. sID= " << _sessionID;
        }
        return;
    }
    LOG(WARNING)<<"TcpSession::close closing. sID=" << _sessionID;
}

int findHead(unsigned char *data,int len){
    if(len<sizeof(MSG_Head))return -1;
    for(int i=0;i<len;++i){
        if(data[i] == MSG_MSG_Head_HEAD)
        {
            if(i+11>=len)return i;
            if(data[i+11] ==MSG_MSG_Head_TAIL )
                return i;
        }
    }
    return -1;
}

unsigned int TcpSession::onRecv(qyhnetwork::NetErrorCode ec, int received)
{
    LOG(TRACE)<<"TcpSession::onRecv sessionID=" << getSessionID() << ", received=" << received;
    if (ec)
    {
        _lastRecvError = ec;
        if (_lastRecvError == NEC_REMOTE_CLOSED)
        {
            LOG(INFO)<<"socket closed.  remote close. sID=" << _sessionID;
        }
        else
        {
            LOG(INFO)<<"socket closed.  socket error(or win rst). sID=" << _sessionID;
        }
        close();
        return 0 ;
    }

    //只输出部分信息
    LOG(INFO)<<"recv:"<<toHexString(read_buffer+read_position,received<sizeof(MSG_Head)?received:sizeof(MSG_Head));

    read_position += received;
    SessionManager::getInstance()->_statInfo[STAT_RECV_COUNT]++;
    SessionManager::getInstance()->_statInfo[STAT_RECV_BYTES] += received;

    //解析
    if(read_position>=sizeof(MSG_Head)){
        int head_position = findHead((unsigned char *)read_buffer,read_position);
        if(head_position == -1){
            //未找到头，丢弃数据
            LOG(INFO)<<" msg head not found , discard read data";
            read_position = 0;
        }else{
            if(head_position != 0){
                memmove(read_buffer, read_buffer + head_position, read_position - head_position );
                read_position -= head_position;
            }
            if(read_position>=sizeof(MSG_Head)){
				memset(&read_one_msg, 0, sizeof(read_one_msg));
                memcpy_s(&read_one_msg,sizeof(MSG_Request),read_buffer,read_position);
                if(read_one_msg.head.body_length<= read_position - sizeof(MSG_Head)){
                    MsgProcess::getInstance()->processOneMsg(read_one_msg,shared_from_this());                
					memmove(read_buffer, read_buffer + sizeof(MSG_Head)+ read_one_msg.head.body_length, read_position - sizeof(MSG_Head)- read_one_msg.head.body_length);
                    read_position -= sizeof(MSG_Head)+read_one_msg.head.body_length;
                }
            }
        }
    }

# ifndef WIN32
    return read_position;
#else
    if (!doRecv())
    {
        close();
    }
    return 0;
#endif

}

void TcpSession::send(const MSG_Response &msg)
{
    SessionManager::getInstance()->_statInfo[STAT_SEND_COUNT]++;
    SessionManager::getInstance()->_statInfo[STAT_SEND_PACKS]++;
    bool sendRet = _sockptr->doSend((char *)&msg,sizeof(MSG_Head)+sizeof(MSG_RESPONSE_HEAD)+msg.head.body_length);
    if (!sendRet)
    {
        LOG(WARNING)<<"send error ";
    }
}

void TcpSession::onPulse()
{
    if (_status == 3)
    {
        return;
    }

    if (_status == 1)
    {
        if (_reconnects >= _options._reconnects)
        {
            if (_options._onReconnectEnd)
            {
                try
                {
                    _options._onReconnectEnd(shared_from_this());
                }
                catch (const std::exception & e)
                {
                    LOG(ERROR)<<"_options._onReconnectEnd catch excetion=" << e.what();
                }
                catch (...)
                {
                    LOG(ERROR)<<"_options._onReconnectEnd catch excetion";
                }
            }
            close();
        }
        else
        {
            reconnect();
            _reconnects++;
            _pulseTimerID = SessionManager::getInstance()->createTimer(_options._connectPulseInterval, std::bind(&TcpSession::onPulse, shared_from_this()));
        }
    }
    else if (_status == 2)
    {
        if (_options._onSessionPulse)
        {
            try
            {
                _options._onSessionPulse(shared_from_this());
            }
            catch (const std::exception & e)
            {
                LOG(WARNING)<<"TcpSession::onPulse catch one exception: " << e.what();
            }
            catch (...)
            {
                LOG(WARNING)<<"TcpSession::onPulse catch one unknown exception: ";
            }
        }
        _pulseTimerID = SessionManager::getInstance()->createTimer(isConnectID(_sessionID) ? _options._connectPulseInterval : _options._sessionPulseInterval, std::bind(&TcpSession::onPulse, shared_from_this()));
    }
}

}
