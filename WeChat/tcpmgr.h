#ifndef TCPMGR_H
#define TCPMGR_H
#include "singleton.h"
#include <QTcpSocket>
#include "global.h"
#include <QTimer>

#define HEARTBEATTIME 30000

/**************************************************************/

//   * @file:      tcpmgr.h
//   * @brife:     tcp长连接，处理客户端之间的消息通讯
//   * @date:      2025/06/10

/**************************************************************/

enum class socketOffStatus{
    CHANGE_LOG_OFF_SOCKET,
    HEART_OUTTIME_OFF_SOCKET,
};


class TcpMgr:public QObject,public Singleton<TcpMgr>{
    Q_OBJECT
    friend class Singleton<TcpMgr>;

private:
    QTcpSocket*_socket;
    QString    _host;
    int16_t    _port;
    QByteArray _buffer;
    bool       _is_recv_pendding;    ///没读完
    quint16    _message_id;
    quint16    _message_len;
    QMap<ReqId,std::function<void(ReqId id,int len,QByteArray data)>> _handlers;

    socketOffStatus _socket_off_status;

    QTimer *_heartbeatTimer;
public:
    ~TcpMgr();
    QTcpSocket* getSocket();
public slots:
    void slot_connect_server(ServerInfo si);                   /// 尝试连接
    void slot_send_data(ReqId reqId, QByteArray data);         /// 发送数据给聊天服务器
    void handleMsg(ReqId id,int len,QByteArray data);          /// 处理回包数据
private:
    TcpMgr();
    void initHandlers();                                       /// 注册回调


signals:
    void sig_socket_connect_success(bool o);            /// socket 连接成功槽函数
    void sig_send_data(ReqId reqId, QByteArray data);   ///对外接口，发送数据请求至服务器
    void sig_switch_chat();                             /// 登录成功的跳转聊天界面函数
    void sig_login_failed(int);                         /// 登陆失败
    void sig_user_search_finish(std::shared_ptr<SearchInfo>);           /// 网络查找用户成功信号
    void sig_user_search_failed(int);                                   /// 网络查找用户失败信号
    void sig_apply_friend_send_result(int);                             ///添加好友回包发射的信号
    void sig_new_friend_apply_comming(std::shared_ptr<FriendRequest>);  /// 收到新的好友请求发送的信号
    void sig_ID_AUTH_FRIEND_RSP_callback(std::shared_ptr<FriendItem>);  /// 同意对方好友请求的回包
    void sig_ID_NOTIFY_AUTH_FRIEND_REQ_callback(std::shared_ptr<FriendItem>);   ///我给对方发送的好友申请，对方同意了的 信号
    void sig_uodateMessage(int uid,QString marking);                    /// 给对方发送消息的信号
    void sig_notify_updateMessage(int selfuid,int touid,QString marking,QString message,int type);  ///对方给我发送消息的处理信号
    void sig_getFriendListFromUid(QList<FriendItem>);                   ///获取好友列表的信号
    void sig_getFriendRequestListFromUid(QList<FriendRequest>list);     /// 获取好友请求列表的信号
    void sig_message_data_to_update(int& ,int& ,QList<MessageItem>);    /// 获取聊天记录后的信号
    void sig_message_data_to_prepend(int& ,QString& ,QString& ,QList<MessageItem>);  /// 获取更早的聊天记录的信号
    void sig_getFileServerInfo(QString ,QString,int is_send);           /// 获取文件服务器信息的信号
    void NotifyOffline(int uid);
    void heartOuttimeOffSocket();

};

#endif // TCPMGR_H
