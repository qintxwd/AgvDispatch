#include "UserManager.h"
#include "sqlite3/CppSQLite3.h"
#include "network/sessionmanager.h"
using namespace qyhnetwork;


UserManager::UserManager()
{
}


UserManager::~UserManager()
{
}

void UserManager::interLogin(TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy_s(&response.head,sizeof(MSG_Head), &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_FAIL;
    response.return_head.error_code = RETURN_MSG_ERROR_CODE_LENGTH;
    if (msg.head.body_length >= 128)
    {
        std::string username(msg.body);
        std::string password(msg.body + 64);

        try{
            CppSQLite3DB db;
            db.open(DB_File);
            CppSQLite3Table table_agv = db.getTable("select id,user_password,user_role,user_signState from agv_user where user_username=?");
            if(table_agv.numRows() == 1)
            {
                table_agv.setRow(0);
                int id = std::atoi(table_agv.fieldValue(0));
                std::string querypwd(table_agv.fieldValue(1));
                int role = std::atoi(table_agv.fieldValue(2));

                if(querypwd == password){
                    conn->setUserId(id);
                    conn->setUserRole(role);

                    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
                    response.return_head.error_code = RETURN_MSG_ERROR_NO_ERROR;
                    USER_INFO u;
                    u.id = id;
                    u.role = role;
                    u.status = 1;
                    memcpy_s(u.username,MSG_STRING_LEN,username.c_str(), username.length());
                    memcpy_s(u.password,MSG_STRING_LEN,password.c_str(), password.length());

                    memcpy_s(response.body,MSG_RESPONSE_BODY_MAX_SIZE, &u, sizeof(u));
                    response.head.body_length = sizeof(u);

                    //更新登录状态
                    std::stringstream ss;
                    ss<<"update agv_user set user_signState=1 where id= "<<u.id;
                    db.execDML(ss.str().c_str());
                }else{
                    response.return_head.error_code = RETURN_MSG_ERROR_CODE_PASSWORD_ERROR;
                }
            }else{
                response.return_head.error_code = RETURN_MSG_ERROR_CODE_USERNAME_NOT_EXIST;
                sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "%s","username not exist");
            }
        }catch(CppSQLite3Exception e){
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "code:%d msg:%s",e.errorCode(),e.errorMessage());
            LOG(ERROR)<<"sqlerr code:"<<e.errorCode()<<" msg:"<<e.errorMessage();
        }catch(std::exception e){
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN,"%s", e.what());
            LOG(ERROR)<<"sqlerr code:"<<e.what();
        }
    }
    //发送返回值
    conn->send(response);
}

void UserManager::interLogout(TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy_s(&response.head,sizeof(MSG_Head), &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    response.return_head.error_code = RETURN_MSG_ERROR_NO_ERROR;

    try{
        CppSQLite3DB db;
        db.open(DB_File);
        std::stringstream ss;
        ss<<"update agv_user set user_signState=1 where id= "<<conn->getUserId();
        db.execDML(ss.str().c_str());
    }catch(CppSQLite3Exception e){
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
        sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "code:%d msg:%s",e.errorCode(),e.errorMessage());
        LOG(ERROR)<<"sqlerr code:"<<e.errorCode()<<" msg:"<<e.errorMessage();
    }catch(std::exception e){
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
        sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN,"%s", e.what());
        LOG(ERROR)<<"sqlerr code:"<<e.what();
    }

    //登出
    conn->setUserId(0);

    //发送返回值
    conn->send(response);

    //断开连接
    SessionManager::getInstance()->kickSession(conn->getSessionID());
}

void UserManager::interChangePassword(TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy_s(&response.head,sizeof(MSG_Head), &(msg.head), sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    response.return_head.error_code = RETURN_MSG_ERROR_NO_ERROR;

    if (msg.head.body_length <= 0 || msg.head.body_length>64) {
        response.return_head.result = RETURN_MSG_RESULT_FAIL;
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_LENGTH;
    }
    else {
        std::string newPassword(msg.body, msg.head.body_length);

        try{
            CppSQLite3DB db;
            db.open(DB_File);
            std::stringstream ss;
            ss<<"update agv_user set user_password="<< newPassword <<" where id = "<<conn->getUserId();
            db.execDML(ss.str().c_str());
            //登出
            conn->setUserId(0);
        }catch(CppSQLite3Exception e){
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "code:%d msg:%s",e.errorCode(),e.errorMessage());
            LOG(ERROR)<<"sqlerr code:"<<e.errorCode()<<" msg:"<<e.errorMessage();
        }catch(std::exception e){
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN,"%s", e.what());
            LOG(ERROR)<<"sqlerr code:"<<e.what();
        }
    }
    //发送返回值
    conn->send(response);

    //断开连接
    SessionManager::getInstance()->kickSession(conn->getSessionID());
}

void UserManager::interList(TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy_s(&response.head,sizeof(MSG_Head), &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    response.return_head.error_code = RETURN_MSG_ERROR_NO_ERROR;
    bool needSendAtLast = true;

    if(conn->getUserRole()<USER_ROLE_ADMIN){
        response.return_head.result = RETURN_MSG_RESULT_FAIL;
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_PERMISSION_DENIED;
    }else{
        try{
            CppSQLite3DB db;
            db.open(DB_File);
            std::stringstream ss;
            ss<<"select id,user_name,user_password,user_role,user_status from agv_user where role <="<<conn->getUserRole();
            CppSQLite3Table table = db.getTable(ss.str().c_str());
            if(table.numRows()>0 && table.numFields() ==5 ){
                for(int i=0;i<table.numRows();++i){
                    table.setRow(i);
                    USER_INFO u;
                    memset(&u, 0, sizeof(USER_INFO));
                    u.id = std::atoi(table.fieldValue(0));
                    sprintf_s(u.username,MSG_STRING_LEN, "%s",table.fieldValue(1));
                    sprintf_s(u.password,MSG_STRING_LEN, "%s",table.fieldValue(2));
                    u.role = std::atoi(table.fieldValue(3));
                    u.status = (uint8_t)std::atoi(table.fieldValue(4));
                    memcpy_s(response.body,MSG_RESPONSE_BODY_MAX_SIZE,&u, sizeof(USER_INFO));
                    response.head.body_length += sizeof(USER_INFO);
                    response.head.body_length = sizeof(USER_INFO);
                    conn->send(response);
                    needSendAtLast = false;
                }
            }
        }catch(CppSQLite3Exception e){
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "code:%d msg:%s",e.errorCode(),e.errorMessage());
            LOG(ERROR)<<"sqlerr code:"<<e.errorCode()<<" msg:"<<e.errorMessage();
        }catch(std::exception e){
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN,"%s", e.what());
            LOG(ERROR)<<"sqlerr code:"<<e.what();
        }
    }

    //发送返回值
    if(needSendAtLast)
        conn->send(response);
}

void UserManager::interRemove(TcpSessionPtr conn, MSG_Request msg)
{
    int deleteid = 0;
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy_s(&response.head, sizeof(MSG_Head), &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    response.return_head.error_code = RETURN_MSG_ERROR_NO_ERROR;
    //数据库查询
    //需要msg中包含一个ID
    if (msg.head.body_length < sizeof(int)) {
        response.return_head.result = RETURN_MSG_RESULT_FAIL;
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_LENGTH;
    }
    else if(conn->getUserRole()<USER_ROLE_ADMIN){
        response.return_head.result = RETURN_MSG_RESULT_FAIL;
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_PERMISSION_DENIED;
    }else{
        uint32_t id = 0;
        memcpy_s(&id, sizeof(uint32_t), msg.body, sizeof(uint32_t));
        try{
            CppSQLite3DB db;
            db.open(DB_File);
            //查询权限
            std::stringstream ss;
            ss<<"select user_role from agv_user where id <="<<id;
            CppSQLite3Table table = db.getTable(ss.str().c_str());
            if(table.numRows()==1){
                table.setRow(0);
                int deleteUserRole = atoi(table.fieldValue(0));
                if(conn->getUserRole() >= deleteUserRole){
                    //执行删除工作
                    std::stringstream ss;
                    ss<<"delete from agv_user where id="<<id;
                    db.execDML(ss.str().c_str());
                    deleteid = id;
                }else{
                    response.return_head.result = RETURN_MSG_RESULT_FAIL;
                    response.return_head.error_code = RETURN_MSG_ERROR_CODE_PERMISSION_DENIED;
                }
            }
        }catch(CppSQLite3Exception e){
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "code:%d msg:%s",e.errorCode(),e.errorMessage());
            LOG(ERROR)<<"sqlerr code:"<<e.errorCode()<<" msg:"<<e.errorMessage();
        }catch(std::exception e){
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN,"%s", e.what());
            LOG(ERROR)<<"sqlerr code:"<<e.what();
        }
    }

    //发送返回值
    conn->send(response);

    //断开 被删除的用户的 连接
    if(deleteid>0){
        SessionManager::getInstance()->kickSessionByUserId(deleteid);
    }
}

void UserManager::interAdd(TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy_s(&response.head,sizeof(MSG_Head), &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    response.return_head.error_code = RETURN_MSG_ERROR_NO_ERROR;

    //需要msg中包含一个ID
    if (msg.head.body_length < sizeof(USER_INFO)) {//这里不需要包含登录状态
        response.return_head.result = RETURN_MSG_RESULT_FAIL;
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_LENGTH;
    }else if(conn->getUserRole()<USER_ROLE_ADMIN){
        response.return_head.result = RETURN_MSG_RESULT_FAIL;
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_PERMISSION_DENIED;
    }
    else {
        USER_INFO u;
        memcpy_s(&u,sizeof(USER_INFO), msg.body, sizeof(USER_INFO) );
        u.status = 0;//在线状态
        if(conn->getUserRole()<u.role){
            response.return_head.result = RETURN_MSG_RESULT_FAIL;
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_PERMISSION_DENIED;
            sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "%s","you cannot add a user with higher permission than you");
        }else{
            try{
                //插入数据库
                CppSQLite3DB db;
                db.open(DB_File);
                std::stringstream ss;
                ss<<"insert into agv_user user_username, user_password,user_role,user_status values("<<u.username <<","<<u.password <<","<<u.role <<",0);";
                db.execDML(ss.str().c_str());

                //获取插入后的ID，返回
                std::stringstream ss2;
                ss2<<"select id from agv_user where user_username = "<<u.username<<" and user_password = "<<u.password<<";";
                int id = db.execScalar(ss2.str().c_str());
                response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
                response.return_head.error_code = RETURN_MSG_ERROR_NO_ERROR;
                memcpy_s(response.body,MSG_RESPONSE_BODY_MAX_SIZE,&id,sizeof(int));
                response.head.body_length = sizeof(int);
            }catch(CppSQLite3Exception e){
                response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
                sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "code:%d msg:%s",e.errorCode(),e.errorMessage());
                LOG(ERROR)<<"sqlerr code:"<<e.errorCode()<<" msg:"<<e.errorMessage();
            }catch(std::exception e){
                response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
                sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN,"%s", e.what());
                LOG(ERROR)<<"sqlerr code:"<<e.what();
            }
        }
    }
    //发送返回值
    conn->send(response);
}

void UserManager::interModify(TcpSessionPtr conn, MSG_Request msg)
{
    MSG_Response response;
    memset(&response, 0, sizeof(MSG_Response));
    memcpy_s(&response.head,sizeof(MSG_Head), &msg.head, sizeof(MSG_Head));
    response.head.body_length = 0;
    response.return_head.result = RETURN_MSG_RESULT_SUCCESS;
    response.return_head.error_code = RETURN_MSG_ERROR_NO_ERROR;

    //需要msg中包含一个ID
    if (msg.head.body_length < sizeof(USER_INFO)) {//这里不需要包含登录状态
        response.return_head.result = RETURN_MSG_RESULT_FAIL;
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_LENGTH;
    }else if(conn->getUserRole()<USER_ROLE_ADMIN){
        response.return_head.result = RETURN_MSG_RESULT_FAIL;
        response.return_head.error_code = RETURN_MSG_ERROR_CODE_PERMISSION_DENIED;
    }
    else {
        USER_INFO u;
        memcpy_s(&u,sizeof(USER_INFO), msg.body, sizeof(USER_INFO));
        if(conn->getUserRole()<u.role){
            response.return_head.result = RETURN_MSG_RESULT_FAIL;
            response.return_head.error_code = RETURN_MSG_ERROR_CODE_PERMISSION_DENIED;
        }else{
            try{
                CppSQLite3DB db;
                db.open(DB_File);
                std::stringstream ss;
                ss<<"update agv_user set user_username="<< u.username<<",user_password=" << u.password <<",user_role="<<u.role <<" where id="<<u.id;
                db.execDML(ss.str().c_str());
            }catch(CppSQLite3Exception e){
                response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
                sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN, "code:%d msg:%s",e.errorCode(),e.errorMessage());
                LOG(ERROR)<<"sqlerr code:"<<e.errorCode()<<" msg:"<<e.errorMessage();
            }catch(std::exception e){
                response.return_head.error_code = RETURN_MSG_ERROR_CODE_QUERY_SQL_FAIL;
                sprintf_s(response.return_head.error_info,MSG_LONG_STRING_LEN,"%s", e.what());
                LOG(ERROR)<<"sqlerr code:"<<e.what();
            }
        }
    }
    //发送返回值
    conn->send(response);
}
