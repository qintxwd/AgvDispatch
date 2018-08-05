#include "tcpacceptor.h"
#include "tcpsession.h"
#include "sessionmanager.h"

TcpAcceptor::TcpAcceptor(boost::asio::io_context &io_context, short port, int _id):
    Acceptor(io_context,port,_id)
{

}

void TcpAcceptor::onAccept(tcp::socket& socket)
{
    TcpSessionPtr pp = std::make_shared<TcpSession>(std::move(socket),SessionManager::getInstance()->getNextSessionId(),getId());
    SessionManager::getInstance()->addSession(pp->getSessionID(),pp);
    pp->start();
}

