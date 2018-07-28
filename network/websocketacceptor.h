#ifndef WEBSOCKETACCEPTOR_H
#define WEBSOCKETACCEPTOR_H

#include "acceptor.h"

class WebsocketAcceptor;
using WebsocketAcceptorPtr = std::shared_ptr<WebsocketAcceptor>;

class WebsocketAcceptor : public Acceptor
{
public:
    WebsocketAcceptor(boost::asio::io_context& io_context, short port,int _id);
    void onAccept(tcp::socket& socket);
};

#endif // WEBSOCKETACCEPTOR_H
