#include "tcpsession.h"
#include "sessionmanager.h"
#include "../common.h"
#include "../msgprocess.h"
#include "agvmanager.h"
#include "../Dongyao/dyforklift.h"

using namespace qyhnetwork;
using std::min;
using std::max;

namespace qyhnetwork {


	TcpSession::TcpSession() :username(""),
		read_len(0),
		json_len(0),
		read_position(0),
		isContinueRecving(false),
		big_msg_buffer(nullptr),
		big_msg_buffer_position(0)
	{
		SessionManager::getInstance()->_statInfo[STAT_SESSION_CREATED]++;
	}

	TcpSession::~TcpSession()
	{
		if (big_msg_buffer) {
			delete big_msg_buffer;
			big_msg_buffer = nullptr;
			big_msg_buffer_position = 0;
		}
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
            combined_logger->error("can't connect on a old session.  please use addConnect try again.");
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
            combined_logger->error( "connect init error");
			return;
		}

		read_position = 0;
		read_len = 0;
		json_len = 0;
		if (big_msg_buffer) {
			delete big_msg_buffer;
			big_msg_buffer = nullptr;
			big_msg_buffer_position = 0;
		}
		isContinueRecving = false;

		if (!_sockptr->doConnect(_remoteIP, _remotePort, std::bind(&TcpSession::onConnected, shared_from_this(), std::placeholders::_1)))
		{
            combined_logger->error("connect error");
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
                combined_logger->error("TcpSession::attatch _onSessionLinked error. e={0}", e.what());
			}
			catch (...)
			{
                combined_logger->warn("TcpSession::attatch _onSessionLinked catch one unknown exception.");
			}
		}

    if(aID == AgvManager::getInstance()->getServerAccepterID())
    {
        DyForkliftPtr agv = std::static_pointer_cast<DyForklift> (AgvManager::getInstance()->getAgvByIP(_remoteIP));

        if(agv)
        {
            agv->setPosition(0, 0, 0);
            agv->status = Agv::AGV_STATUS_NOTREADY;
            agv->setQyhTcp(_sockptr);
            agv->setPort(_remotePort);
            //start report
            agv->startReport(100);
		if (!doRecv())
		{
			close();
			return false;
		}
            setAGVPtr(agv);
	}
        else
        {
            combined_logger->warn("AGV is not in the list::ip={}.", _remoteIP);
        }
    }
    else
    {

        if (!doRecv())
        {
            close();
            return false;
        }
    }
    return true;
}
	void TcpSession::onConnected(qyhnetwork::NetErrorCode ec)
	{
		if (ec)
		{
            combined_logger->warn("onConnected error. ec={0},  cID={1}" ,ec ,_sessionID);
			return;
		}
        combined_logger->info( "onConnected success. sessionID={0}",_sessionID);

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
                combined_logger->error("TcpSession::onConnected error. e={0}", e.what());
			}
			catch (...)
			{
                combined_logger->warn("TcpSession::onConnected catch one unknown exception.");
			}
		}
		SessionManager::getInstance()->_statInfo[STAT_SESSION_LINKED]++;
	}

	bool TcpSession::doRecv()
	{
        //combined_logger->trace() << "TcpSession::doRecv sessionID=" << getSessionID();
		if (!_sockptr)
		{
			return false;
		}
#ifndef WIN32
		return _sockptr->doRecv(read_buffer, MSG_READ_BUFFER_LENGTH, std::bind(&TcpSession::onRecv, shared_from_this(), std::placeholders::_1, std::placeholders::_2), true);
#else
		return _sockptr->doRecv(read_buffer + read_position, MSG_READ_BUFFER_LENGTH - read_position, std::bind(&TcpSession::onRecv, shared_from_this(), std::placeholders::_1, std::placeholders::_2));
#endif
	}

	void TcpSession::close()
	{
		if (_status != 3 && _status != 0)
		{
			if (_sockptr)
			{
				_sockptr->doClose();
				//_sockptr.reset();
			}
            combined_logger->info("TcpSession to close socket. sID= {0}",_sessionID);
			if (_status == 2)
			{
				SessionManager::getInstance()->_statInfo[STAT_SESSION_CLOSED]++;
				if (_options._onSessionClosed)
				{
					SessionManager::getInstance()->post(std::bind(_options._onSessionClosed, shared_from_this()));
				}
			}

        if(_acceptID == AgvManager::getInstance()->getServerAccepterID())
        {
            //AgvManager::getInstance()->removeAgv(_agvPtr);
        }

			if (isConnectID(_sessionID) && _reconnects < _options._reconnects)
			{
				_status = 1;
                combined_logger->info("TcpSession already closed. try reconnect ... sID= {0}" , _sessionID);
			}
			else
			{
				_status = 3;
				SessionManager::getInstance()->post(std::bind(&SessionManager::removeSession, SessionManager::getInstance(), shared_from_this()));
                combined_logger->info("TcpSession remove self from manager. sID= {0}" ,_sessionID);
			}
			return;
		}
        combined_logger->warn("TcpSession::close closing. sID={0}", _sessionID);
	}

int TcpSession::ProtocolProcess()
{
    int StartIndex,EndIndex;
    std::string FrameData;
    int start_byteFlag = receivedBuffer.find('*');
    int end_byteFlag = receivedBuffer.find('#');
    if((start_byteFlag ==-1)&&(end_byteFlag ==-1))
    {
        receivedBuffer.clear();
        return 5;
    }
    else if((start_byteFlag == -1)&&(end_byteFlag != -1))
    {
        receivedBuffer.clear();
        return 6;
    }
    else if((start_byteFlag != -1)&&(end_byteFlag == -1))
    {
        receivedBuffer = receivedBuffer.substr(receivedBuffer.find('*'));
        return 7;
    }
    StartIndex = receivedBuffer.find('*')+1;
    EndIndex = receivedBuffer.find('#');
    if(EndIndex>StartIndex)
    {
        FrameData = receivedBuffer.substr(StartIndex,EndIndex-StartIndex);
        if(FrameData.find('*') != -1)
            FrameData = FrameData.substr(FrameData.find_last_of('*')+1);
        receivedBuffer= receivedBuffer.substr(receivedBuffer.find('#')+1);
    }
    else
    {
        //remove unuse message
        receivedBuffer= receivedBuffer.substr(receivedBuffer.find('*'));
        return 2;
    }
    int FrameLength = std::stoi(FrameData.substr(6,4));
    if(FrameLength == FrameData.length())
    {
        std::static_pointer_cast<DyForklift>(_agvPtr)->onRead(FrameData.c_str(), FrameLength);
        return 0;
    }
    else
    {
        return 1;
    }
}
	unsigned int TcpSession::onRecv(qyhnetwork::NetErrorCode ec, int received)
	{
		if (ec)
		{
			_lastRecvError = ec;
			if (_lastRecvError == NEC_REMOTE_CLOSED)
			{
                combined_logger->info("socket closed.  remote close. sID=", _sessionID);
			}
			else
			{
                combined_logger->info("socket closed.  socket error(or win rst). sID=" , _sessionID);
			}
			close();
			return 0;
		}

        //combined_logger->trace() << "TcpSession::onRecv sessionID=" << getSessionID() << ", received=" << received;

		SessionManager::getInstance()->_statInfo[STAT_RECV_COUNT]++;
		SessionManager::getInstance()->_statInfo[STAT_RECV_BYTES] += received;

    //    combined_logger->info("{0}, msglen:{1}", _acceptID, received);
    if(_acceptID !=  AgvManager::getInstance()->getServerAccepterID())
    {
		read_len += received;
		read_position += received;

		//分包、黏包处理{包长 为 json_len+5 }
		while (true) {
			if (isContinueRecving) {
				if (read_len >= json_len + 5) {
					//接收完成，甚至超出
					memcpy(big_msg_buffer + big_msg_buffer_position, read_buffer, received - read_len + (json_len + 5));
					
					std::string json_str = std::string(big_msg_buffer, json_len);
                    //                    combined_logger->trace("RECV! session id={0}  len={1} json=\n{2}" ,this->_sessionID,json_len,json_str);

					Json::Reader reader;
					Json::Value root;

					if (reader.parse(json_str, root))
					{
						//必备要素检查
						if (!root["type"].isNull() && !root["queuenumber"].isNull() && !root["todo"].isNull()) {
							//处理消息数据
							MsgProcess::getInstance()->processOneMsg(root, shared_from_this());
						}
					}
					
					delete big_msg_buffer;
					big_msg_buffer = nullptr;
					big_msg_buffer_position = 0;

					memmove(read_buffer, read_buffer + received + json_len + 5 - read_len, read_len - 5 - json_len);
					read_len -= json_len + 5;
					isContinueRecving = false;
					read_position = read_len;
					json_len = 0;
				}
				else {
					//接收未完成
					memcpy(big_msg_buffer + big_msg_buffer_position, read_buffer, received);
					big_msg_buffer_position += received;
					read_position = 0;
					break;
				}
			}
			else {
				if ((read_buffer[0] & 0xFF) == MSG_MSG_HEAD && read_len >= 5) {
					snprintf((char *)&json_len, sizeof(json_len), read_buffer + 1, sizeof(int32_t));
					if (json_len > 0) {//找到长度信息
						if (read_len >= json_len + 5) {//判断这个消息是否完整

							std::string json_str = std::string(read_buffer + 5, json_len);

                            //                            combined_logger->trace("RECV! session id={0} len={1} json=\n{2}",this->_sessionID,json_len, json_str);
;
							Json::Reader reader;
							Json::Value root;
							
							if (reader.parse( json_str, root))
							{
								//必备要素检查
								if (!root["type"].isNull() && !root["queuenumber"].isNull() && !root["todo"].isNull()) {
									//处理消息数据
									MsgProcess::getInstance()->processOneMsg(root, shared_from_this());
								}
							}
							memmove(read_buffer, read_buffer + 5 + json_len, read_len - 5 - json_len);
							read_len -= 5 + json_len;
							read_position -= 5 + json_len;
							json_len = 0;
						}
						else if (json_len <= MSG_JSON_MEMORY_LENGTH) {
							//没有超出一个json的最大缓存
							//继续接收就可以了
							break;
						}
						else {
							//如果超出了，那么需要写文件
							if (big_msg_buffer) {
								delete big_msg_buffer;
								big_msg_buffer = nullptr;
								big_msg_buffer_position = 0;
							}

							big_msg_buffer = new char[json_len + 1];
							
							//2.写入json数据
							memcpy(big_msg_buffer, read_buffer + 5, read_len-5);
							big_msg_buffer_position += read_len - 5;

							//3.置位
							read_position = 0;
							isContinueRecving = true;
							break;
						}
					}
					else {
						//数据长度不正确，//忽略数据
						read_position = 0;
						read_len = 0;
						json_len = 0;
						break;
					}
				}
				else {
					//在未找到头信息时，开头还不是头信息，//忽略数据
					read_position = 0;
					read_len = 0;
					json_len = 0;
					break;
			}
		}
        }
    }
    else
    {
        receivedBuffer.append(read_buffer, received);

        int returnValue;
        do
        {
            returnValue = ProtocolProcess();
        }while(receivedBuffer.size() >12 && (returnValue == 0||returnValue == 2||returnValue == 1 ));
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

	void TcpSession::send(const Json::Value &json)
	{
		SessionManager::getInstance()->_statInfo[STAT_SEND_COUNT]++;
		SessionManager::getInstance()->_statInfo[STAT_SEND_PACKS]++;
		std::string msg = json.toStyledString();
		int length = msg.length();
		
    //    combined_logger->trace("SEND! session id={0}  len= {1}  json=\n{2}" ,this->_sessionID,length, msg);

		char *send_buffer = new char[length + 6];
		memset(send_buffer, 0, length + 6);
		send_buffer[0] = MSG_MSG_HEAD;
		memcpy_s(send_buffer + 1, length+5, (char *)&length, sizeof(length));
		memcpy_s(send_buffer +5, length + 1, msg.c_str(),msg.length());
		bool sendRet = _sockptr->doSend(send_buffer, length+5);
		if (!sendRet)
		{
            combined_logger->warn("send error ") ;
		}
		delete[] send_buffer;
	}


void TcpSession::send(char*msg, int length)
{
    char headLeng[5];
    headLeng[0] = MSG_MSG_HEAD;
    SessionManager::getInstance()->_statInfo[STAT_SEND_COUNT]++;
    SessionManager::getInstance()->_statInfo[STAT_SEND_PACKS]++;

    //    combined_logger->trace("SEND! session id={0}  len= {1}  json=\n{2}" ,this->_sessionID,length, msg);

    if(_sockptr==nullptr)return ;
    bool sendRet = _sockptr->doSend(msg, length);
    if (!sendRet)
    {
        combined_logger->warn("send error ") ;
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
                        combined_logger->error("_options._onReconnectEnd catch excetion={0}" , e.what()) ;
					}
					catch (...)
					{
                        combined_logger->error("_options._onReconnectEnd catch excetion");
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
                    combined_logger->warn("TcpSession::onPulse catch one exception: {0}",e.what());
				}
				catch (...)
				{
                    combined_logger->warn("TcpSession::onPulse catch one unknown exception: ");
				}
			}
			_pulseTimerID = SessionManager::getInstance()->createTimer(isConnectID(_sessionID) ? _options._connectPulseInterval : _options._sessionPulseInterval, std::bind(&TcpSession::onPulse, shared_from_this()));
		}
	}

}
