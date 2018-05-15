#include "tcpsession.h"
#include "sessionmanager.h"
#include "../common.h"
#include "../msgprocess.h"

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
			LOG(ERROR) << "can't connect on a old session.  please use addConnect try again.";
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
			LOG(ERROR) << "connect init error";
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
			LOG(ERROR) << "connect error";
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
				LOG(ERROR) << "TcpSession::attatch _onSessionLinked error. e=" << e.what();
			}
			catch (...)
			{
				LOG(WARNING) << "TcpSession::attatch _onSessionLinked catch one unknown exception.";
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
			LOG(WARNING) << "onConnected error. ec=" << ec << ",  cID=" << _sessionID;
			return;
		}
		LOG(INFO) << "onConnected success. sessionID=" << _sessionID;

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
				LOG(ERROR) << "TcpSession::onConnected error. e=" << e.what();
			}
			catch (...)
			{
				LOG(WARNING) << "TcpSession::onConnected catch one unknown exception.";
			}
		}
		SessionManager::getInstance()->_statInfo[STAT_SESSION_LINKED]++;
	}

	bool TcpSession::doRecv()
	{
		LOG(TRACE) << "TcpSession::doRecv sessionID=" << getSessionID();
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
				_sockptr.reset();
			}
			LOG(INFO) << "TcpSession to close socket. sID= " << _sessionID;
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
				LOG(INFO) << "TcpSession already closed. try reconnect ... sID= " << _sessionID;
			}
			else
			{
				_status = 3;
				SessionManager::getInstance()->post(std::bind(&SessionManager::removeSession, SessionManager::getInstance(), shared_from_this()));
				LOG(INFO) << "TcpSession remove self from manager. sID= " << _sessionID;
			}
			return;
		}
		LOG(WARNING) << "TcpSession::close closing. sID=" << _sessionID;
	}

	unsigned int TcpSession::onRecv(qyhnetwork::NetErrorCode ec, int received)
	{
		if (ec)
		{
			_lastRecvError = ec;
			if (_lastRecvError == NEC_REMOTE_CLOSED)
			{
				LOG(INFO) << "socket closed.  remote close. sID=" << _sessionID;
			}
			else
			{
				LOG(INFO) << "socket closed.  socket error(or win rst). sID=" << _sessionID;
			}
			close();
			return 0;
		}

		LOG(TRACE) << "TcpSession::onRecv sessionID=" << getSessionID() << ", received=" << received;

		SessionManager::getInstance()->_statInfo[STAT_RECV_COUNT]++;
		SessionManager::getInstance()->_statInfo[STAT_RECV_BYTES] += received;

		read_len += received;
		read_position += received;

		//分包、黏包处理{包长 为 json_len+5 }
		while (true) {
			if (isContinueRecving) {
				if (read_len >= json_len + 5) {
					//接收完成，甚至超出
					memcpy(big_msg_buffer + big_msg_buffer_position, read_buffer, received - read_len + (json_len + 5));
					
					//big_msg_buffer_position += received - read_len + (json_len + 5);
					
					Json::CharReaderBuilder builder;
					builder["collectComments"] = false;
					Json::CharReader* reader(builder.newCharReader());
					JSONCPP_STRING errs;
					Json::Value root;
					if (reader->parse(big_msg_buffer, big_msg_buffer+json_len, &root, &errs))
					{
						//必备要素检查
						if (!root["type"].isNull() && !root["queuenumber"].isNull() && !root["todo"].isNull()) {
							//处理消息数据
							MsgProcess::getInstance()->processOneMsg(root, shared_from_this());
						}
					}
					delete reader;
					
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
				if (read_buffer[0] == MSG_MSG_HEAD && read_len >= 5) {
					snprintf((char *)&json_len, sizeof(json_len), read_buffer + 1, sizeof(int32_t));
					if (json_len > 0) {//找到长度信息
						if (read_len >= json_len + 5) {//判断这个消息是否完整
							//独立的简单的json数据
							Json::CharReaderBuilder builder;
							builder["collectComments"] = false;
							Json::CharReader* reader(builder.newCharReader());
							JSONCPP_STRING errs;
							Json::Value root;
							if (reader->parse(read_buffer + 5, read_buffer + 5 + json_len, &root, &errs))
							{
								//必备要素检查
								if (!root["type"].isNull() && !root["queuenumber"].isNull() && !root["todo"].isNull()) {
									//处理消息数据
									MsgProcess::getInstance()->processOneMsg(root, shared_from_this());
								}
							}
							delete reader;
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
		char headLeng[5];
		headLeng[0] = 0xAA;
		SessionManager::getInstance()->_statInfo[STAT_SEND_COUNT]++;
		SessionManager::getInstance()->_statInfo[STAT_SEND_PACKS]++;
		std::string msg = json.toStyledString();
		int length = msg.length();
		//send head and length
		snprintf(headLeng + 1, 4, (char *)&length, sizeof(length));
		bool sendRet = _sockptr->doSend(headLeng, 5);
		if (!sendRet)
		{
			LOG(WARNING) << "send error ";
			return;
		}

		char *copy_temp = new char[length + 1];
		snprintf(copy_temp,length+1, "%s", msg.c_str());

		sendRet = _sockptr->doSend(copy_temp, length);
		if (!sendRet)
		{
			LOG(WARNING) << "send error ";
		}
		delete[] copy_temp;
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
						LOG(ERROR) << "_options._onReconnectEnd catch excetion=" << e.what();
					}
					catch (...)
					{
						LOG(ERROR) << "_options._onReconnectEnd catch excetion";
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
					LOG(WARNING) << "TcpSession::onPulse catch one exception: " << e.what();
				}
				catch (...)
				{
					LOG(WARNING) << "TcpSession::onPulse catch one unknown exception: ";
				}
			}
			_pulseTimerID = SessionManager::getInstance()->createTimer(isConnectID(_sessionID) ? _options._connectPulseInterval : _options._sessionPulseInterval, std::bind(&TcpSession::onPulse, shared_from_this()));
		}
	}

}
