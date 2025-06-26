#include "tcpfilemgr.h"

tcpFilemgr::~tcpFilemgr()
{

}

QTcpSocket *tcpFilemgr::getSocket()
{
    return _socket;
}

bool tcpFilemgr::isFinishConnect()
{
    return is_connect;
}

bool tcpFilemgr::slot_connect_server(QString &host, QString &port,int is_send)
{
    qDebug()<<"tcpMgr try to connect the file server...";
    _host = host;
    _port = static_cast<quint16>(port.toUInt());

    this->is_send = is_send;

    qDebug()<<"chatserver host is :"<<_host;
    qDebug()<<"chatserver port is :"<<_port;
    _socket->connectToHost(_host, _port);
    return true;
}

void tcpFilemgr::sendMsg(ReqId req, QByteArray data)
{
    //发送信号，统一交给槽函数处理，这么做的好处是多线程安全
    emit sig_send_msg(req, data);
}

tcpFilemgr::tcpFilemgr():_host(""),_port(0),_is_recv_pendding(false),_message_id(0),_message_len(0),
    is_send(0),is_connect(false){

    _socket = new QTcpSocket(this);

    connect(_socket, &QTcpSocket::connected,[this](){
        qDebug() << "成功连接到文件服务器：" << _host << ":" << _port;
        is_connect = true;
        emit socketConnectChange(true, is_send);
    });

    connect(_socket, &QTcpSocket::errorOccurred, [this](){
        qDebug() << "连接文件服务器失败：" << _socket->errorString();
        is_connect = false;
        emit socketConnectChange(false, is_send);
    });

    connect(_socket,&QTcpSocket::readyRead,[this](){
        this->slot_ready_read();
    });




    connect(this, &tcpFilemgr::sig_send_msg, this, &tcpFilemgr::slot_send_msg);
}

void tcpFilemgr::initHandlers()
{

}

void tcpFilemgr::slot_send_msg(ReqId id, QByteArray body)
{
    ///qDebug()<<"inter the send file slot...";

    //如果连接异常则直接返回
    if(_socket->state() != QAbstractSocket::ConnectedState){
        qDebug() << "socket 连接异常..." << _socket->errorString();
        is_connect = false;
        emit socketConnectChange(false,0);
        return;
    }

    //获取body的长度
    quint32 bodyLength = body.size();

    //创建字节数组
    QByteArray data;
    //绑定字节数组
    QDataStream stream(&data, QIODevice::WriteOnly);
    //设置大端模式
    stream.setByteOrder(QDataStream::BigEndian);


    // 显式转换为 quint16 保证写入 2 字节
    quint16 shortId = static_cast<quint16>(id);
    stream << shortId;        // 2 字节
    stream << bodyLength;     // 4 字节

    data.append(body);        // body 可变长度

    //发送消息
    _socket->write(data);

}

void tcpFilemgr::slot_ready_read()           /// 发送文件是服务器回包的槽函数
{
    //读取所有数据
    QByteArray data = _socket->readAll();

    //将数据缓存起来
    _buffer.append(data);

    //处理收到的数据
    processData();
}

void tcpFilemgr::processData()
{
    while(_buffer.size() >= TCP_HEAD_LEN){
        ///先取出六字节头部
        auto head_byte = _buffer.left(TCP_HEAD_LEN);
        QDataStream stream(head_byte);

        ///设置为大端模式
        stream.setByteOrder(QDataStream::BigEndian);

        quint16 msg_id;          ///读取ID
        stream >> msg_id;

        quint32 body_length;     ///读取长度
        stream >> body_length;

        if(_buffer.size() >= TCP_HEAD_LEN+body_length){
            ///完整的消息体已经接受
            QByteArray body = _buffer.mid(TCP_HEAD_LEN,body_length);
            ///去掉完整的消息包
            _buffer = _buffer.mid(TCP_HEAD_LEN+body_length);
            /// 解析服务器发过来的消息
            QJsonDocument jsonDoc = QJsonDocument::fromJson(body);

            if (jsonDoc.isNull() || !jsonDoc.isObject()) {
                qDebug() << "Invalid JSON, skipping.";
                continue;                        /// 继续处理下一个包，不断开连接
            }

            /// qDebug() << "receive data is " << body;

            /// 获取 JSON 对象
            QJsonObject jsonObject = jsonDoc.object();

            ///qDebug() << "Received msg_id:" << msg_id;
            emit sig_logic_process(msg_id, jsonObject);
        }else{
            //消息未完全接受，所以中断
            break;
        }
    }
}
