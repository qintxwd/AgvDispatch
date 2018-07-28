#include "tcpsession.h"
#include "../protocol.h"
#include "../common.h"
#include "../msgprocess.h"
#include "sessionmanager.h"

TcpSession::TcpSession(tcp::socket socket, int sessionId):
    Session(sessionId),
    socket_(std::move(socket)),
    json_len(0)
{
    combined_logger->debug("new connection from {0}:{1} sessionId={2} ", socket_.remote_endpoint().address().to_string(), socket_.remote_endpoint().port(),sessionId);
}
TcpSession::~TcpSession()
{
    close();
}

void TcpSession::send(const Json::Value &json)
{
    std::string msg = json.toStyledString();
    int length = msg.length();
    if(length<=0)return;

    combined_logger->info("SEND! session id={0}  len= {1} json={2}" ,this->_sessionID,length,msg);

    char *send_buffer = new char[length + 6];
    memset(send_buffer, 0, length + 6);
    send_buffer[0] = MSG_MSG_HEAD;
    memcpy_s(send_buffer + 1, length+5, (char *)&length, sizeof(length));
    memcpy_s(send_buffer +5, length + 1, msg.c_str(),msg.length());

    int per_send_length = 1300;

    if(length+5<per_send_length){
        //if(length+5 !=  boost::asio::write(socket_, boost::asio::buffer(data, len)));
        if(length+5 != boost::asio::write(socket_,boost::asio::buffer(send_buffer,length+5)))
        {
            combined_logger->warn("send error ") ;
        }
    }else{
        int packindex = 0;
        char *per_send_buffer = new char[per_send_length];
        int resendTime = 3;
        while(true){
            int this_time_send_length = (length +5) - packindex * per_send_length;
            if(this_time_send_length<=0)break;
            if(this_time_send_length > per_send_length)this_time_send_length = per_send_length;

            memcpy(per_send_buffer,send_buffer+packindex*per_send_length,this_time_send_length);
            bool sendRet = false;
            for(int i=0;i<resendTime;++i){
                if(this_time_send_length == socket_.write_some(boost::asio::buffer(per_send_buffer,this_time_send_length))){
                    sendRet = true;
                    break;
                }
            }

            if(!sendRet){
                combined_logger->warn("send error ") ;
                break;
            }

            ++packindex;
        }

        delete[] per_send_buffer;

    }
    delete[] send_buffer;
}

bool TcpSession::doSend(const char *data,int len)
{
    if(!data || len<=0)return false;

    return len == boost::asio::write(socket_, boost::asio::buffer(data, len));
}

void TcpSession::close()
{
    socket_.close();
}

void TcpSession::start()
{
    std::thread([this](){
        while(socket_.is_open()){
            boost::system::error_code error;
            size_t length = socket_.read_some(boost::asio::buffer(read_buffer,MSG_READ_BUFFER_LENGTH), error);
            if (error == boost::asio::error::eof){
                combined_logger->info("session close cleany,session id:{0}",getSessionID());
                break;
            }
            else if (error){
                combined_logger->info("session error!session id:{0},error:{1}",getSessionID(),error.message());
                break;
            }
            packageProcess(read_buffer,length);
        }
        SessionManager::getInstance()->removeSession(shared_from_this());
    }).detach();
}

void TcpSession::packageProcess(const char *data,int len)
{
    buffer.append(data,len);
    if(buffer.size()<=json_len){
        return ;
    }

    while(true)
    {
        if(buffer.length()<=0)break;
        //寻找包头
        int pack_head = buffer.find(MSG_MSG_HEAD);
        if(pack_head>=0){
            //丢弃前面的无用数据
            buffer.removeFront(pack_head);
            //json长度
            json_len = buffer.getInt32(1);
            if(json_len==0){
                //空包,清除包头和长度信息。
                buffer.removeFront(1+sizeof(int32_t));
            }else if(json_len>0){
                if(json_len <= buffer.length()-1-sizeof(int32_t))
                {
                    //包完整
                    //处理包数据
                    Json::Reader reader(Json::Features::strictMode());
                    Json::Value root;
                    std::string sss = std::string(buffer.data(1+sizeof(int32_t)),json_len);
                    if (reader.parse(sss, root))
                    {
                        if (!root["type"].isNull() && !root["queuenumber"].isNull() && !root["todo"].isNull()) {
                            combined_logger->trace("RECV! session id={0}  len={1} json=\n{2}" ,this->_sessionID,json_len,sss);
                            MsgProcess::getInstance()->processOneMsg(root, shared_from_this());
                        }
                    }
                    //清除报数据
                    buffer.removeFront(1+sizeof(int32_t)+json_len);
                    json_len = 0;
                }else{
                    //包不完整，继续接收
                    break;
                }
            }
        }else{
            //未找到包头
            buffer.clear();
            break;
        }
    }
}
