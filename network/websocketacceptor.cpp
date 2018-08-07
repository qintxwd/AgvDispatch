#include "websocketacceptor.h"
#include "websocketsession.h"
#include "sessionmanager.h"

WebsocketAcceptor::WebsocketAcceptor(boost::asio::io_context &io_context, short port, int _id):
    Acceptor(io_context,port,_id)
{

}

void WebsocketAcceptor::onAccept(tcp::socket& socket)
{
    WebSocketSessionPtr pp = std::make_shared<WebSocketSession>(std::move(socket),SessionManager::getInstance()->getNextSessionId());
    SessionManager::getInstance()->addSession(pp->getSessionID(),pp);
    pp->start();
}
