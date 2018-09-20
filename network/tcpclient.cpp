#include "tcpclient.h"
#include <iostream>

TcpClient::TcpClient(std::string _ip, int _port, ClientReadCallback _readcallback, ClientConnectCallback _connectcallback, ClientDisconnectCallback _disconnectcallback):
    ip(_ip),
    port(_port),
    need_reconnect(true),
    readcallback(_readcallback),
    connectcallback(_connectcallback),
    disconnectcallback(_disconnectcallback),
    //    s(io_context),
    quit(false)
{
}


void TcpClient::start()
{
    t =  std::thread(std::bind(&TcpClient::threadProcess,this));
}

TcpClient::~TcpClient()
{
    need_reconnect = false;
    quit = true;
    doClose();
    if(t.joinable())
        t.join();
}


void TcpClient::threadProcess()
{
    char recvBuf[QYH_TCP_CLIENT_RECV_BUF_LEN];

    while(!quit)
    {
        if(!need_reconnect){
            std::this_thread::sleep_for(std::chrono::duration<int,std::milli>(50));
            continue;
        }
        try
        {
            //客户端重连需要
            s.reset(new boost::asio::ip::tcp::socket(io_context));
            boost::asio::ip::address add;
            add.from_string(ip);
            boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(ip), port);
            s->connect(endpoint);

            if(connectcallback)connectcallback();

            while(!quit && s->is_open()){
                boost::system::error_code ec;
                size_t recv_length;
                try
                {
                    recv_length = s->available();
                }catch(...)
                {
                    std::cout<<"read error";
                }
                //                if(recv_length<=0)continue;

                recv_length = s->read_some(boost::asio::buffer(recvBuf, QYH_TCP_CLIENT_RECV_BUF_LEN),ec);
                if(ec){
                    std::cout<<"ec = "<<ec.message()<<" code="<<ec.value();
                    break;
                }else{
                    if(recv_length>0 && readcallback)
                    {
                        readcallback(recvBuf,recv_length);
                    }
                }
            }
            if(disconnectcallback)disconnectcallback();
        }catch (std::exception &e)
        {
            //std::cerr<<"Exception:"<<e.what()<<std::endl;
            if(disconnectcallback)disconnectcallback();
        }
        //re connect duration: 2000ms
        std::this_thread::sleep_for(std::chrono::duration<int,std::milli>(2000));
    }
}

void TcpClient::resetConnect(std::string _ip, int _port)
{
    disconnect();
    ip = _ip;
    port = _port;
    need_reconnect = true;
}

bool TcpClient::sendToServer(const char *data,int len)
{
    try{
        return len == s->write_some(boost::asio::buffer(data, len));
    }catch(...){
        return false;
    }

}

void TcpClient::disconnect()
{
    need_reconnect = true;
    s->shutdown(tcp::socket::shutdown_both);
    s->close();
}

void TcpClient::doClose()
{
    s->close();
}



