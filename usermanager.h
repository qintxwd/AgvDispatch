#pragma once

#include "common.h"
#include "protocol.h"
#include "utils/noncopyable.h"
#include "network/tcpsession.h"
using qyhnetwork::TcpSessionPtr;

class UserManager;
using UserManagerPtr = std::shared_ptr<UserManager>;

class UserManager :public noncopyable, public std::enable_shared_from_this<UserManager>
{
public:
    static UserManagerPtr getInstance()
	{
        static UserManagerPtr m_inst = UserManagerPtr(new UserManager());
		return m_inst;
	}

    void init();

    void interLogin(TcpSessionPtr conn, const Json::Value &request);

    void interLogout(TcpSessionPtr conn, const Json::Value &request);

    void interChangePassword(TcpSessionPtr conn, const Json::Value &request);

    void interList(TcpSessionPtr conn, const Json::Value &request);

    void interRemove(TcpSessionPtr conn, const Json::Value &request);

    void interAdd(TcpSessionPtr conn, const Json::Value &request);

    void interModify(TcpSessionPtr conn, const Json::Value &request);

	virtual ~UserManager();
private:
	UserManager();

    void checkTable();
};

