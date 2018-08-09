#include "websocketsession.h"
#include <functional>
#include "sessionmanager.h"
#include "../common.h"
#include "../msgprocess.h"

WebSocketSession::WebSocketSession(tcp::socket socket, int sessionId, int acceptId):
    Session(sessionId,acceptId),
    ws(std::move(socket))
{
    combined_logger->info("new websocket connection sessionId={0} ", sessionId);
}

WebSocketSession::~WebSocketSession()
{
    close();
}

void WebSocketSession::read()
{
    try
    {
        ws.accept();

        //MsgProcess::getInstance()->addSubTask(getSessionID());

        while(ws.is_open())
        {
            boost::beast::multi_buffer buffer;

            ws.read(buffer);

            std::string ss = boost::beast::buffers_to_string(buffer.data());
            Json::Reader reader(Json::Features::strictMode());
            Json::Value root;
            if (reader.parse(ss, root))
            {
                if (!root["type"].isNull() && !root["queuenumber"].isNull() && !root["todo"].isNull())
                {
                    //combined_logger->info("RECV! session id={0}  len={1} json=\n{2}" ,this->_sessionID,ss.length(),ss);
                    MsgProcess::getInstance()->processOneMsg(root, shared_from_this());
                }
            }
        }
    }
    catch(boost::system::system_error const& se)
    {
        combined_logger->debug("error websocket session id={0}  Error={2}" ,this->_sessionID,se.code().message());
    }
    catch(std::exception const& e)
    {
        combined_logger->debug("error websocket session id={0}  Error={2}" ,this->_sessionID,e.what());
    }


    SessionManager::getInstance()->removeSession(shared_from_this());
}


void WebSocketSession::start()
{
    std::thread(std::bind(&WebSocketSession::read,this)).detach();
}


void WebSocketSession::send(const Json::Value &json)
{
    boost::system::error_code err;
	std::string sss = json.toStyledString();
    //combined_logger->info("SEND! session id={0}  len= {1} json={2}", this->_sessionID, sss.length(), sss.length());
    ws.write(boost::asio::buffer(sss.c_str(), sss.length()),err);
}

bool WebSocketSession::doSend(const char *data,int len)
{
    boost::system::error_code err;
    return len ==  ws.write(boost::asio::buffer(data,len),err);
}

void WebSocketSession::close()
{
    try{
        ws.close(boost::beast::websocket::close_code::normal);
    }catch(...){

    }
}

