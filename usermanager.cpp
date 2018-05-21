#include "usermanager.h"
#include "sqlite3/CppSQLite3.h"
#include "network/sessionmanager.h"
#include "userlogmanager.h"
using namespace qyhnetwork;


UserManager::UserManager()
{
}


UserManager::~UserManager()
{
}

void UserManager::checkTable() {
	//检查表
	try {
		if (!g_db.tableExists("agv_user")) {
			g_db.execDML("create table agv_user(id INTEGER primary key AUTOINCREMENT, user_password char(64),user_username char(64),user_role INTEGER,user_status INTEGER);");
		}
	}
	catch (CppSQLite3Exception e) {
		combined_logger->error() << "sqlerr code:" << e.errorCode() << " msg:" << e.errorMessage();
		return;
	}
	catch (std::exception e) {
		combined_logger->error() << "sqlerr code:" << e.what();
		return;
	}
}


void UserManager::init()
{
	checkTable();
}


void UserManager::interLogin(TcpSessionPtr conn, const Json::Value &request)
{
	Json::Value response;
	response["type"] = MSG_TYPE_RESPONSE;
	response["todo"] = request["todo"];
	response["queuenumber"] = request["queuenumber"];
	response["result"] = RETURN_MSG_RESULT_SUCCESS;

	if (request["username"].isNull() ||
		request["password"].isNull()) {
		response["error_code"] = RETURN_MSG_ERROR_CODE_PARAMS;
	}
	else {
		std::string username = request["username"].asString();
		std::string password = request["password"].asString();
		std::stringstream ss;
		ss << "select id,user_password,user_role,user_status from agv_user where user_username=\'" << username << "\'";
		try {
			CppSQLite3Table table_agv = g_db.getTable(ss.str().c_str());
			if (table_agv.numRows() == 1)
			{
				table_agv.setRow(0);
				int id = std::atoi(table_agv.fieldValue(0));
				std::string querypwd(table_agv.fieldValue(1));
				int role = std::atoi(table_agv.fieldValue(2));

				if (querypwd == password) {
					conn->setUserId(id);
					conn->setUserName(username);
					conn->setUserRole(role);

					response["result"] = RETURN_MSG_RESULT_SUCCESS;
					response["error_code"] = RETURN_MSG_ERROR_NO_ERROR;

					response["id"] = id;
					response["role"] = role;
					response["status"] = 1;
					response["username"] = username;
					response["password"] = password;

					UserLogManager::getInstance()->push(username + " login success");
					//更新登录状态
					std::stringstream ss;
					ss << "update agv_user set user_status=1 where id= " << id;
					g_db.execDML(ss.str().c_str());
				}
				else {
					response["error_code"] = RETURN_MSG_ERROR_CODE_PASSWORD_ERROR;
				}
			}
			else {
				response["error_code"] = RETURN_MSG_ERROR_CODE_USERNAME_NOT_EXIST;
			}
		}
		catch (CppSQLite3Exception e) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
			std::stringstream ss;
			ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
			response["error_info"] = ss.str();
			combined_logger->error() << "sqlerr code:" << e.errorCode() << " msg:" << e.errorMessage();
		}
		catch (std::exception e) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
			std::stringstream ss;
			ss << "info:" << e.what();
			response["error_info"] = ss.str();
			combined_logger->error() << "sqlerr code:" << e.what();
		}
	}

	//发送返回值
	conn->send(response);
}

void UserManager::interLogout(TcpSessionPtr conn, const Json::Value &request)
{
	Json::Value response;
	response["type"] = MSG_TYPE_RESPONSE;
	response["todo"] = request["todo"];
	response["queuenumber"] = request["queuenumber"];
	response["result"] = RETURN_MSG_RESULT_SUCCESS;

	try {
		UserLogManager::getInstance()->push(conn->getUserName() + " logout");
		std::stringstream ss;
		ss << "update agv_user set user_status=1 where id= " << conn->getUserId();
		g_db.execDML(ss.str().c_str());
	}
	catch (CppSQLite3Exception e) {
		response["result"] = RETURN_MSG_RESULT_FAIL;
		response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
		std::stringstream ss;
		ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
		response["error_info"] = ss.str();
		combined_logger->error() << "sqlerr code:" << e.errorCode() << " msg:" << e.errorMessage();
	}
	catch (std::exception e) {
		response["result"] = RETURN_MSG_RESULT_FAIL;
		response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
		std::stringstream ss;
		ss << "info:" << e.what();
		response["error_info"] = ss.str();
		combined_logger->error() << "sqlerr code:" << e.what();
	}

	//登出
	conn->setUserId(0);

	//发送返回值
	conn->send(response);

	//断开连接
	SessionManager::getInstance()->kickSession(conn->getSessionID());
}

void UserManager::interChangePassword(TcpSessionPtr conn, const Json::Value &request)
{
	Json::Value response;
	response["type"] = MSG_TYPE_RESPONSE;
	response["todo"] = request["todo"];
	response["queuenumber"] = request["queuenumber"];
	response["result"] = RETURN_MSG_RESULT_SUCCESS;

	if (request["password"].isNull()) {
		response["error_code"] = RETURN_MSG_ERROR_CODE_PARAMS;
	}
	else {
		std::string newPassword = request["password"].asString();
		UserLogManager::getInstance()->push(conn->getUserName() + " change password");
		try {
			std::stringstream ss;
			ss << "update agv_user set user_password=" << newPassword << " where id = " << conn->getUserId();
			g_db.execDML(ss.str().c_str());
			//登出
			conn->setUserId(0);
		}
		catch (CppSQLite3Exception e) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
			std::stringstream ss;
			ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
			response["error_info"] = ss.str();
			combined_logger->error() << "sqlerr code:" << e.errorCode() << " msg:" << e.errorMessage();
		}
		catch (std::exception e) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
			std::stringstream ss;
			ss << "info:" << e.what();
			response["error_info"] = ss.str();
			combined_logger->error() << "sqlerr code:" << e.what();
		}
	}
	//发送返回值
	conn->send(response);

	//断开连接
	SessionManager::getInstance()->kickSession(conn->getSessionID());
}

void UserManager::interList(TcpSessionPtr conn, const Json::Value &request)
{
	Json::Value response;
	response["type"] = MSG_TYPE_RESPONSE;
	response["todo"] = request["todo"];
	response["queuenumber"] = request["queuenumber"];
	response["result"] = RETURN_MSG_RESULT_SUCCESS;

	UserLogManager::getInstance()->push(conn->getUserName() + " query users list");
	if (conn->getUserRole() < USER_ROLE_ADMIN) {
		response["result"] = RETURN_MSG_RESULT_FAIL;
		response["error_code"] = RETURN_MSG_ERROR_CODE_PERMISSION_DENIED;
	}
	else {
		try {
			std::stringstream ss;
			ss << "select id,user_username,user_password,user_role,user_status from agv_user where user_role <=" << conn->getUserRole();
			CppSQLite3Table table = g_db.getTable(ss.str().c_str());
			Json::Value users;
			if (table.numRows() > 0 && table.numFields() == 5) {
				for (int i = 0; i < table.numRows(); ++i) {
					Json::Value user;
					table.setRow(i);
					user["id"] = std::atoi(table.fieldValue(0));
					user["username"] = std::string(table.fieldValue(1));
					user["password"] = std::string(table.fieldValue(2));
					user["role"] = std::atoi(table.fieldValue(3));
					user["status"] = std::atoi(table.fieldValue(4));
					users.append(user);
				}
			}
			response["users"] = users;
		}
		catch (CppSQLite3Exception e) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
			std::stringstream ss;
			ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
			response["error_info"] = ss.str();
			combined_logger->error() << "sqlerr code:" << e.errorCode() << " msg:" << e.errorMessage();
		}
		catch (std::exception e) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
			std::stringstream ss;
			ss << "info:" << e.what();
			response["error_info"] = ss.str();
			combined_logger->error() << "sqlerr code:" << e.what();
		}
	}

	//发送返回值
	conn->send(response);
}

void UserManager::interRemove(TcpSessionPtr conn, const Json::Value &request)
{
	int deleteid = 0;

	Json::Value response;
	response["type"] = MSG_TYPE_RESPONSE;
	response["todo"] = request["todo"];
	response["queuenumber"] = request["queuenumber"];
	response["result"] = RETURN_MSG_RESULT_SUCCESS;

	//数据库查询
	//需要msg中包含一个ID
	if (request["id"].isNull()) {
		response["result"] = RETURN_MSG_RESULT_FAIL;
		response["error_code"] = RETURN_MSG_ERROR_CODE_PARAMS;
	}
	else if (conn->getUserRole() < USER_ROLE_ADMIN) {
		response["result"] = RETURN_MSG_RESULT_FAIL;
		response["error_code"] = RETURN_MSG_ERROR_CODE_PERMISSION_DENIED;
	}
	else {
		int id = request["id"].asInt();
		try {
			//查询权限
			std::stringstream ss;
			ss << "select user_role from agv_user where id =" << id;
			CppSQLite3Table table = g_db.getTable(ss.str().c_str());
			if (table.numRows() == 1) {
				table.setRow(0);
				int deleteUserRole = atoi(table.fieldValue(0));
				if (conn->getUserRole() >= deleteUserRole) {
					//执行删除工作
					std::stringstream ss;
					ss << "delete from agv_user where id=" << id;
					UserLogManager::getInstance()->push(conn->getUserName() + " delete user " + intToString(id));
					g_db.execDML(ss.str().c_str());
					deleteid = id;
				}
				else {
					response["result"] = RETURN_MSG_RESULT_FAIL;
					response["error_code"] = RETURN_MSG_ERROR_CODE_PERMISSION_DENIED;
				}
			}
			else {
				response["result"] = RETURN_MSG_RESULT_FAIL;
				response["error_code"] = RETURN_MSG_ERROR_CODE_PERMISSION_DENIED;
			}
		}
		catch (CppSQLite3Exception e) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
			std::stringstream ss;
			ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
			response["error_info"] = ss.str();
			combined_logger->error() << "sqlerr code:" << e.errorCode() << " msg:" << e.errorMessage();
		}
		catch (std::exception e) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
			std::stringstream ss;
			ss << "info:" << e.what();
			response["error_info"] = ss.str();
			combined_logger->error() << "sqlerr code:" << e.what();
		}
	}

	//发送返回值
	conn->send(response);

	//断开 被删除的用户的 连接
	if (deleteid > 0) {
		SessionManager::getInstance()->kickSessionByUserId(deleteid);
	}
}

void UserManager::interAdd(TcpSessionPtr conn, const Json::Value &request)
{
	Json::Value response;
	response["type"] = MSG_TYPE_RESPONSE;
	response["todo"] = request["todo"];
	response["queuenumber"] = request["queuenumber"];
	response["result"] = RETURN_MSG_RESULT_SUCCESS;

	//需要msg中包含一个ID

	if (request["username"].isNull()
		|| request["password"].isNull()
		|| request["role"].isNull()) {
		response["result"] = RETURN_MSG_RESULT_FAIL;
		response["error_code"] = RETURN_MSG_ERROR_CODE_PARAMS;
	}
	else if (conn->getUserRole() < USER_ROLE_ADMIN) {
		response["result"] = RETURN_MSG_RESULT_FAIL;
		response["error_code"] = RETURN_MSG_ERROR_CODE_PERMISSION_DENIED;
	}
	else {
		int role = request["role"].asInt();
		if (conn->getUserRole() < role) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_PERMISSION_DENIED;
			response["error_info"] = std::string("you cannot add a user with higher permission than you");
		}
		else {
			try {
				//插入数据库
				std::stringstream ss;
				ss << "insert into agv_user (user_username, user_password,user_role,user_status) values('" << request["username"].asString() << "','" << request["password"].asString() << "'," << request["role"].asInt() << ",0);";
				g_db.execDML(ss.str().c_str());

				//获取插入后的ID，返回
				std::stringstream ss2;
				ss2 << "select max(id) from agv_user ;";
				int id = g_db.execScalar("select max(id) from agv_user ;");
				UserLogManager::getInstance()->push(conn->getUserName() + " add user with id:" + intToString(id) + " username:" + request["username"].asString());
				response["id"] = id;
			}
			catch (CppSQLite3Exception e) {
				response["result"] = RETURN_MSG_RESULT_FAIL;
				response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
				std::stringstream ss;
				ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
				response["error_info"] = ss.str();
				combined_logger->error() << "sqlerr code:" << e.errorCode() << " msg:" << e.errorMessage();
			}
			catch (std::exception e) {
				response["result"] = RETURN_MSG_RESULT_FAIL;
				response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
				std::stringstream ss;
				ss << "info:" << e.what();
				response["error_info"] = ss.str();
				combined_logger->error() << "sqlerr code:" << e.what();
			}
		}
	}
	//发送返回值
	conn->send(response);
}

void UserManager::interModify(TcpSessionPtr conn, const Json::Value &request)
{
	Json::Value response;
	response["type"] = MSG_TYPE_RESPONSE;
	response["todo"] = request["todo"];
	response["queuenumber"] = request["queuenumber"];
	response["result"] = RETURN_MSG_RESULT_SUCCESS;

	//需要msg中包含一个ID
	if (request["id"].isNull()
		|| request["username"].isNull()
		|| request["password"].isNull()
		|| request["role"].isNull()) {
		response["result"] = RETURN_MSG_RESULT_FAIL;
		response["error_code"] = RETURN_MSG_ERROR_CODE_PARAMS;
	}
	else if (conn->getUserRole() < USER_ROLE_ADMIN) {
		response["result"] = RETURN_MSG_RESULT_FAIL;
		response["error_code"] = RETURN_MSG_ERROR_CODE_PERMISSION_DENIED;
	}
	else {
		int id = request["id"].asInt();
		std::string username = request["username"].asString();
		std::string password = request["password"].asString();
		int role = request["role"].asInt();
		if (conn->getUserRole() < role) {
			response["result"] = RETURN_MSG_RESULT_FAIL;
			response["error_code"] = RETURN_MSG_ERROR_CODE_PERMISSION_DENIED;
		}
		else {
			try {
				UserLogManager::getInstance()->push(conn->getUserName() + " modify user info with id " + intToString(id));
				std::stringstream ss;
				ss << "update agv_user set user_username=" << username << ",user_password=" << password << ",user_role=" << role << " where id=" << id;
				g_db.execDML(ss.str().c_str());
			}
			catch (CppSQLite3Exception e) {
				response["result"] = RETURN_MSG_RESULT_FAIL;
				response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
				std::stringstream ss;
				ss << "code:" << e.errorCode() << " msg:" << e.errorMessage();
				response["error_info"] = ss.str();
				combined_logger->error() << "sqlerr code:" << e.errorCode() << " msg:" << e.errorMessage();
			}
			catch (std::exception e) {
				response["result"] = RETURN_MSG_RESULT_FAIL;
				response["error_code"] = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
				std::stringstream ss;
				ss << "info:" << e.what();
				response["error_info"] = ss.str();
				combined_logger->error() << "sqlerr code:" << e.what();
			}
		}
	}
	//发送返回值
	conn->send(response);
}
