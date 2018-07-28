#pragma once

#include "common.h"
#include "protocol.h"
#include "utils/noncopyable.h"
#include "network/session.h"

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

    void interLogin(SessionPtr conn, const Json::Value &request);

    void interLogout(SessionPtr conn, const Json::Value &request);

    void interChangePassword(SessionPtr conn, const Json::Value &request);

    void interList(SessionPtr conn, const Json::Value &request);

    void interRemove(SessionPtr conn, const Json::Value &request);

    void interAdd(SessionPtr conn, const Json::Value &request);

    void interModify(SessionPtr conn, const Json::Value &request);

	virtual ~UserManager();
private:
	UserManager();

    void checkTable();
};

