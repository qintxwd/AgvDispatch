#ifndef ACCEPTOR_H
#define ACCEPTOR_H

#include <boost/asio.hpp>
#include <thread>

using boost::asio::ip::tcp;

class Acceptor;
using AcceptorPtr = std::shared_ptr<Acceptor>;

class Acceptor : public std::enable_shared_from_this<Acceptor>
{
public:
    Acceptor(boost::asio::io_context& io_context, short port,int _id);
    virtual ~Acceptor();
    virtual void onAccept(tcp::socket& socket) = 0;

    inline int getId(){return id;}
    inline void setId(int _id){id = _id;}
    virtual void start();
    virtual void close();
    virtual void doAccept();
protected:
    tcp::acceptor acceptor_;
    int id;
    boost::asio::io_context& _io_context;
    std::thread t;
};

#endif // ACCEPTOR_H
