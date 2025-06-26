#ifndef TCPFILEMGR_H
#define TCPFILEMGR_H

#include "singleton.h"
#include <QTcpSocket>
#include "global.h"

/**************************************************************/

//   * @file:      tcpfilemgr.h
//   * @brife:     上传文件的tcpMgr，连接至聊天服务器实现上传和下载文件
//   * @date:      2025/06/10

/**************************************************************/

class tcpFilemgr : public QObject,public Singleton<tcpFilemgr>{
    Q_OBJECT
    friend class Singleton<tcpFilemgr>;

private:
    QTcpSocket*_socket;
    QString    _host;
    quint16    _port;
    QByteArray _buffer;
    bool       _is_recv_pendding;    ///没读完
    quint16    _message_id;
    quint16    _message_len;


    bool is_send;

    QMap<ReqId,std::function<void(ReqId id,int len,QByteArray data)>> _handlers;

    bool is_connect;                 /// 标志位，标识是否连接到 fileserver
public:
    ~tcpFilemgr();
    QTcpSocket* getSocket();

    bool isFinishConnect();     ///是否连接上
    bool slot_connect_server(QString& host,QString& port,int is_send);   ///尝试连接

    void sendMsg(ReqId req ,QByteArray data);   /// 发送一个文件包片段

private:
    tcpFilemgr();
    void initHandlers();

    void slot_send_msg(ReqId , QByteArray data);  /// 发送一个文件包片段

    void slot_ready_read();       ///socket 读取到有数据时触发的函数，用于处理服务器的数据
    void processData();

signals:
    void socketConnectChange(bool,int );      /// socket连接状态改变
    void sig_send_msg(ReqId req , QByteArray data);
    void sig_logic_process(quint16 msg_id, const QJsonObject& jsonObj);   ///发送文件服务器回包
};

#endif // TCPFILEMGR_H
