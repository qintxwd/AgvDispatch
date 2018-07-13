#include "websocketserver.h"

#include "QtWebSockets/qwebsocketserver.h"
#include "QtWebSockets/qwebsocket.h"
#include <QtCore/QDebug>

QT_USE_NAMESPACE

/********************************************************************************************************
 * Function Name :                  WebSocketServer
 * Function Main Usage:             构造函数
 *
 * Author/Date:
 * Modefy/Date:
********************************************************************************************************/
WebSocketServer::WebSocketServer(quint16 port, bool debug, QObject *parent) :
    QObject(parent),
    m_pWebSocketServer(new QWebSocketServer(QStringLiteral("Echo Server"),
                                            QWebSocketServer::NonSecureMode, this)),
    m_debug(debug)
{
    if (m_pWebSocketServer->listen(QHostAddress::Any, port)) {
        if (m_debug)
            qDebug() << "WebSocketServer listening on port" << port;
        connect(m_pWebSocketServer, &QWebSocketServer::newConnection,
                this, &WebSocketServer::onNewConnection);
        connect(m_pWebSocketServer, &QWebSocketServer::closed, this, &WebSocketServer::closed);
    }
}
/********************************************************************************************************
 * Function Name :                  ~WebSocketServer
 * Function Main Usage:             析构函数
 *
 * Author/Date:
 * Modefy/Date:
********************************************************************************************************/
WebSocketServer::~WebSocketServer()
{
    m_pWebSocketServer->close();
    qDeleteAll(m_clients.begin(), m_clients.end());
}
/********************************************************************************************************
 * Function Name :                  onNewConnection
 * Function Main Usage:             新连接
 *
 * Author/Date:
 * Modefy/Date:
********************************************************************************************************/
void WebSocketServer::onNewConnection()
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();

    connect(pSocket, &QWebSocket::textMessageReceived, this, &WebSocketServer::processTextMessage);
    connect(pSocket, &QWebSocket::binaryMessageReceived, this, &WebSocketServer::processBinaryMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &WebSocketServer::socketDisconnected);

    m_clients << pSocket;
}
/********************************************************************************************************
 * Function Name :                  processTextMessage
 * Function Main Usage:             处理文本信息
 *
 * Author/Date:
 * Modefy/Date:
********************************************************************************************************/

void WebSocketServer::processTextMessage(QString message)
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (m_debug)
        qDebug() << "Message received:" << message;

    emit sig_websocket_msg(message, pClient);
}

/********************************************************************************************************
 * Function Name :                  processBinaryMessage
 * Function Main Usage:             处理二值化信息
 *
 * Author/Date:
 * Modefy/Date:
********************************************************************************************************/

void WebSocketServer::processBinaryMessage(QByteArray message)
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (m_debug)
        qDebug() << "Binary Message received:" << message;
    if (pClient) {
        pClient->sendBinaryMessage(message);
    }
}

/********************************************************************************************************
 * Function Name :                  socketDisconnected
 * Function Main Usage:             关闭连接
 *
 * Author/Date:
 * Modefy/Date:
********************************************************************************************************/

void WebSocketServer::socketDisconnected()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    if (m_debug)
        qDebug() << "socketDisconnected:" << pClient;
    if (pClient) {
        m_clients.removeAll(pClient);
        pClient->deleteLater();
    }
}

/********************************************************************************************************
 * Function Name :                  slot_sendResponse
 * Function Main Usage:             发送应答消息
 *
 * Author/Date:
 * Modefy/Date:
********************************************************************************************************/

void WebSocketServer::slot_sendResponse(QString message, QWebSocket* client)
{
    if(client)
    {
        client->sendTextMessage(message);
    }
}
