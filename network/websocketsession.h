#ifndef WEBSOCKETSESSION_H
#define WEBSOCKETSESSION_H

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <string>
#include <thread>

#include "session.h"

using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
namespace websocket = boost::beast::websocket;  // from <boost/beast/websocket.hpp>

class WebSocketSession;
using WebSocketSessionPtr = std::shared_ptr<WebSocketSession>;


class WebSocketSession : public Session
{
public:
    WebSocketSession(tcp::socket socket,int sessionId);

    virtual ~WebSocketSession();

    virtual void start();

    virtual void send(const Json::Value &json);

    virtual bool doSend(const char *data,int len);

    virtual void close();

    void read();

private:
    websocket::stream<tcp::socket> ws;
};

#endif // WEBSOCKETSESSION_H
