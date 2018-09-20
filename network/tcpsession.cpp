#include "tcpsession.h"
#include "../protocol.h"
#include "../common.h"
#include "../msgprocess.h"
#include "sessionmanager.h"
#include "../agvmanager.h"
#include "../Dongyao/dyforklift.h"
#include "../Anting/atforklift.h"
using std::min;
using std::max;

TcpSession::TcpSession(tcp::socket socket, int sessionId, int acceptId):
    Session(sessionId,acceptId),
    socket_(std::move(socket)),
    json_len(0)
{
    combined_logger->debug("new connection from {0}:{1} sessionId={2} ", socket_.remote_endpoint().address().to_string(), socket_.remote_endpoint().port(),sessionId);
}

TcpSession::~TcpSession()
{
    //close();
}

void TcpSession::send(const Json::Value &json)
{
    std::string msg = json.toStyledString();
    int length = msg.length();
    if(length<=0)return;

    //combined_logger->info("SEND! session id={0}  len= {1} json={2}" ,this->_sessionID,length,msg);

    char *send_buffer = new char[length + 6];
    memset(send_buffer, 0, length + 6);
    send_buffer[0] = MSG_MSG_HEAD;
    memcpy_s(send_buffer + 1, length+5, (char *)&length, sizeof(length));
    memcpy_s(send_buffer +5, length + 1, msg.c_str(),msg.length());

    int per_send_length = MSG_READ_BUFFER_LENGTH;

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

    if(GLOBAL_AGV_PROJECT == AGV_PROJECT_ANTING || GLOBAL_AGV_PROJECT == AGV_PROJECT_DONGYAO){
        if(getAcceptID() == AgvManager::getInstance()->getServerAccepterID()){
            //this is a agv connection
            if(!attach())return ;
        }
    }

    std::thread([this](){
        while(socket_.is_open()){
            boost::system::error_code error;
            size_t length = socket_.read_some(boost::asio::buffer(read_buffer,MSG_READ_BUFFER_LENGTH), error);
            if (error == boost::asio::error::eof){
                combined_logger->info("session close cleany,session id:{0}",getSessionID());
                if(AGV_PROJECT_DONGYAO == GLOBAL_AGV_PROJECT && _agvPtr != nullptr)
                {
                    std::static_pointer_cast<DyForklift>(_agvPtr)->setQyhTcp(nullptr);
                }
                break;
            }
            else if (error){
                combined_logger->info("session error!session id:{0},error:{1}",getSessionID(),error.message());
                if(AGV_PROJECT_DONGYAO == GLOBAL_AGV_PROJECT && _agvPtr != nullptr)
                {
                    std::static_pointer_cast<DyForklift>(_agvPtr)->setQyhTcp(nullptr);
                }
                break;
            }
            buffer.append(read_buffer,length);

            if(_acceptID !=  AgvManager::getInstance()->getServerAccepterID())
            {
                packageProcess();
            }else{
                int returnValue;
                do
                {
                    returnValue = ProtocolProcess();
                }while(buffer.size() >12 && (returnValue == 0||returnValue == 2||returnValue == 1 ));
            }
        }
        SessionManager::getInstance()->removeSession(shared_from_this());
        if(AGV_PROJECT_DONGYAO == GLOBAL_AGV_PROJECT && _agvPtr != nullptr)
        {
            std::static_pointer_cast<DyForklift>(_agvPtr)->setQyhTcp(nullptr);
        }
    }).detach();
}

int TcpSession::ProtocolProcess()
{
    int StartIndex,EndIndex;
    std::string FrameData;
    int start_byteFlag = buffer.find('*');
    int end_byteFlag = buffer.find('#');
    if((start_byteFlag ==-1)&&(end_byteFlag ==-1))
    {
        buffer.clear();
        return 5;
    }
    else if((start_byteFlag == -1)&&(end_byteFlag != -1))
    {
        buffer.clear();
        return 6;
    }
    else if((start_byteFlag != -1)&&(end_byteFlag == -1))
    {
        buffer.removeFront(start_byteFlag);
        return 7;
    }
    StartIndex = buffer.find('*')+1;
    EndIndex = buffer.find('#');
    if(EndIndex>StartIndex)
    {
        FrameData = buffer.substr(StartIndex,EndIndex-StartIndex);
        if(FrameData.find('*') != std::string::npos)
            FrameData = FrameData.substr(FrameData.find_last_of('*')+1);
        buffer.removeFront(buffer.find('#')+1);
    }
    else
    {
        //remove unuse message
        buffer.removeFront(buffer.find('*'));
        return 2;
    }
    if(FrameData.length()<10)return 1;
    unsigned int FrameLength = stringToInt(FrameData.substr(6,4));
    if(FrameLength == FrameData.length())
    {
        if(GLOBAL_AGV_PROJECT == AGV_PROJECT_ANTING)
        {
            std::static_pointer_cast<AtForklift>(_agvPtr)->onRead(FrameData.c_str(), FrameLength);
        }
        else if(GLOBAL_AGV_PROJECT == AGV_PROJECT_DONGYAO)
        {
            std::static_pointer_cast<DyForklift>(_agvPtr)->onRead(FrameData.c_str(), FrameLength);
        }
        return 0;
    }
    else
    {
        return 1;
    }
}

void TcpSession::packageProcess()
{
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
                    Json::Reader reader(Json::Features::strictMode());
                    Json::Value root;
                    std::string sss = std::string(buffer.data(1+sizeof(int32_t)),json_len);
                    if (reader.parse(sss, root))
                    {
                        if (!root["type"].isNull() && !root["queuenumber"].isNull() && !root["todo"].isNull()) {
                            //combined_logger->info("RECV! session id={0}  len={1} json=\n{2}" ,this->_sessionID,json_len,sss);
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

bool TcpSession::attach()
{
    if(AGV_PROJECT_DONGYAO == GLOBAL_AGV_PROJECT)
    {
        auto agv = std::static_pointer_cast<DyForklift> (AgvManager::getInstance()->getAgvByIP(socket_.remote_endpoint().address().to_string()));

        if(agv)
        {
            agv->setPosition(0, 0, 0);
            agv->status = Agv::AGV_STATUS_NOTREADY;
            agv->setQyhTcp(shared_from_this());
            agv->setPort(socket_.remote_endpoint().port());
            //start report
            agv->startReport(100);
            setAGVPtr(agv);
            return true;
        }
        else
        {
            combined_logger->warn("AGV is not in the list::ip={}.", socket_.remote_endpoint().address().to_string());
            return false;
        }
    }
    else if(AGV_PROJECT_ANTING == GLOBAL_AGV_PROJECT)
    {
        auto agv = std::static_pointer_cast<AtForklift>(AgvManager::getInstance()->getAgvByIP(socket_.remote_endpoint().address().to_string()));
        if(agv!=nullptr){
            agv->setPosition(0, 0, 0);
            agv->status = Agv::AGV_STATUS_NOTREADY;
            agv->setQyhTcp(shared_from_this());
            agv->setPort(socket_.remote_endpoint().port());
            //start report
            agv->startReport(100);
            setAGVPtr(agv);

            return true;
        }else{
            combined_logger->info("there is no agv with this ip:{}",socket_.remote_endpoint().address().to_string());
        }
    }
    return false;
}
