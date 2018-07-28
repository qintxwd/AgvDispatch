#ifndef TCPACCEPTOR_H
#define TCPACCEPTOR_H

#include "acceptor.h"

class TcpAcceptor;
using TcpAcceptorPtr = std::shared_ptr<TcpAcceptor>;

class TcpAcceptor : public Acceptor
{
public:
    TcpAcceptor(boost::asio::io_context& io_context, short port,int _id);
    void onAccept(tcp::socket& socket);
};

#endif // TCPACCEPTOR_H
