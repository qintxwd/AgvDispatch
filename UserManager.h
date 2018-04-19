#pragma once

#include <memory>
#include "Common.h"
#include "Protocol.h"
#include "utils/noncopyable.h"
#include "network/tcpsession.h"
using qyhnetwork::TcpSessionPtr;

class UserManager :private noncopyable, public std::enable_shared_from_this<UserManager>
{
public:
    typedef std::shared_ptr<UserManager> Pointer;

    //通过连接，查找用户信息
    typedef std::unordered_map<int, USER_INFO> MapIdUser;
    typedef std::shared_ptr<MapIdUser> MapIdUserPointer;

	static Pointer getInstance()
	{
		static Pointer m_inst = Pointer(new UserManager());
		return m_inst;
	}

	void saveUser(USER_INFO &u)
	{
		(*m_idUsers)[u.id] = u;
	}

	void removeUser(int id)
	{
		m_idUsers->erase(id);
	}

	MapIdUserPointer getUserMap()
	{
		return m_idUsers;
	}

    void interLogin(TcpSessionPtr conn, MSG_Request msg);

    void interLogout(TcpSessionPtr conn, MSG_Request msg);

    void interChangePassword(TcpSessionPtr conn, MSG_Request msg);

    void interList(TcpSessionPtr conn, MSG_Request msg);

    void interRemove(TcpSessionPtr conn, MSG_Request msg);

    void interAdd(TcpSessionPtr conn, MSG_Request msg);

    void interModify(TcpSessionPtr conn, MSG_Request msg);

	virtual ~UserManager();
private:
	UserManager();
	MapIdUserPointer m_idUsers;
};

