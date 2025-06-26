#include "tcpmgr.h"
#include <QDataStream>
#include <QAbstractSocket>
#include "usermgr.h"
#include <QJsonArray>

TcpMgr::~TcpMgr(){
    _socket->deleteLater();
}

void TcpMgr::slot_connect_server(ServerInfo si){
    qDebug()<<"tcpMgr try to connect the chat server...";
    _host = si.Host;
    _port = static_cast<quint16>(si.Port.toUInt());

    qDebug()<<"chatserver host is :"<<_host;
    qDebug()<<"chatserver port is :"<<_port;

    if (_socket->state() == QAbstractSocket::ConnectedState) {
        qDebug() << "tcpMgr: already connected, disconnecting first...";
        _socket->disconnectFromHost();
        if (_socket->state() != QAbstractSocket::UnconnectedState) {
            // 如果仍未断开，则等待断开完成
            if (!_socket->waitForDisconnected(3000)) {
                qWarning() << "tcpMgr: failed to disconnect from previous server.";
            }
        }
    }

    _socket->connectToHost(_host,_port);
}

//发送数据至聊天服务器端
void TcpMgr::slot_send_data(ReqId reqId, QByteArray dataBytes){
    quint16 id = static_cast<quint16>(reqId);
    // 获取真实字节长度
    quint16 len = static_cast<quint16>(dataBytes.size());
    // 创建 block 数据包
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::BigEndian);
    // 写入消息头（id + len）
    out << id << len;
    // 写入消息体
    block.append(dataBytes);
    // 发送
    _socket->write(block);

    //qDebug()<<"已发送验证至聊天服务器...";
}

void TcpMgr::handleMsg(ReqId id, int len, QByteArray data)
{
    if(_handlers.find(id) == _handlers.end()){
        qDebug()<<"not find the handlers function ";
        return ;
    }
    _handlers[id](id, len ,data);
}


TcpMgr::TcpMgr():_host(""),_port(0),_is_recv_pendding(false),_message_id(0),_message_len(0),
    _socket_off_status(socketOffStatus::HEART_OUTTIME_OFF_SOCKET) {

    _socket = new QTcpSocket(this);

    _heartbeatTimer = new QTimer(this);
    connect(_heartbeatTimer, &QTimer::timeout, [this](){
        QJsonObject obj;
        obj["uid"] = UserMgr::getInstance()->getUid();
        QJsonDocument docu(obj);
        QByteArray byte = docu.toJson(QJsonDocument::Compact);
        emit this->sig_send_data(ReqId::ID_HEART_BEAT_MESSAGE_REQ,byte);
    });

    connect(_socket,&QTcpSocket::connected,[&](){
        ///qDebug()<<"socket connected_1 signals";

        /// 设置下一次socket 断开连接的枚举类型
        _socket_off_status = socketOffStatus::HEART_OUTTIME_OFF_SOCKET;
        _heartbeatTimer->start(HEARTBEATTIME);

        emit sig_socket_connect_success(true);
    });
    connect(_socket,&QTcpSocket::disconnected,[&](){
        ///qDebug()<<"socket disconnected_2 signals";
        _heartbeatTimer->stop();
        /// 断开连接的两种情况： 1. 重新登陆是socket主动断开旧连接   2. 服务器检测到该会话心跳超时断开连接
        if(_socket_off_status == socketOffStatus::CHANGE_LOG_OFF_SOCKET){
            qDebug()<<"socket 因为重新登陆而触发的一次断开连接信号...";
            /// 这里没必要设置枚举值了，因为在断开连接之后会链接新服务器，连接成功后会设置枚举值
            return ;
        }
        if(_socket_off_status == socketOffStatus::HEART_OUTTIME_OFF_SOCKET){
            qDebug()<<"socket 因为客户端心跳超时被服务器断开连接而触发的一次断开连接信号...";

            /// 发送心跳超时断开连接的信号，断连重登逻辑
            emit heartOuttimeOffSocket();
            return;
        }
    });
    connect(_socket,&QTcpSocket::errorOccurred,[&](QAbstractSocket::SocketError socketError){
        switch (socketError) {
        case QAbstractSocket::HostNotFoundError:
        case QAbstractSocket::ConnectionRefusedError:
        case QAbstractSocket::SocketTimeoutError:
        case QAbstractSocket::NetworkError:
            qDebug() << "连接失败:" << _socket->errorString();
            emit sig_socket_connect_success(false);
            break;
        default:
            qDebug() << "非连接失败类错误:" << socketError << _socket->errorString();
            break;
        }
    });

    ///socket 从tcp 缓冲区读到数据后触发（读聊天服务器发送来的消息）
    connect(_socket,&QTcpSocket::readyRead,[this](){

        qDebug()<<"readyRead slots run ...";
        _buffer.append(_socket->readAll());

        QDataStream stream(&_buffer,QIODevice::ReadOnly);
        stream.setVersion(QDataStream::Qt_6_5);

        forever{
            if(!_is_recv_pendding){        ///没有数据没读完，开始读新的一条数据
                if(_buffer.size() <= static_cast<int>(sizeof(quint16) * 2)){
                    return;
                }
                ///数据头4字节完整
                stream >> _message_id >> _message_len;
                _buffer = _buffer.mid(sizeof(quint16) * 2);
            }
            //数据长度不够
            if(_buffer.size() < _message_len){
                _is_recv_pendding = true; return;
            }
            //数据长度够了
            _is_recv_pendding = false;
            QByteArray mess = _buffer.mid(0,_message_len);
            _buffer = _buffer.mid(_message_len);

            ///解析出一条完整的数据后： 这条数据是什么请求类型，请求的长度，请求体
            qDebug()<<"reqid is："<<ReqId(_message_id);
            handleMsg(ReqId(_message_id),_message_len,mess);
            qDebug()<<"handle a data message...";
        }
    });

    QObject::connect(this, &TcpMgr::sig_send_data, this, &TcpMgr::slot_send_data);
    initHandlers();
}

void TcpMgr::initHandlers(){
    //注册获取登录回包逻辑
    _handlers.insert(ReqId::ID_CHAT_LOGIN_RSP, [this](ReqId id,int len,QByteArray data){
        Q_UNUSED(len);
        Q_UNUSED(id);
        ///qDebug()<<"----------------------------------------------------------login handle id is "<<id <<"  data is "<<data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if(jsonDoc.isNull()){
            qDebug()<<"failed to create QJsonDocument...";
            emit sig_login_failed(0);
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if(!jsonObj.contains("error")){
            int err = ErrorCodes::ERR_JSON;
            qDebug()<<"login faliled, err is "<<err;
            emit sig_login_failed(err);            ////发送登陆失败的信号
            return;
        }
        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug()<<"login faliled, err is "<<err;
            emit sig_login_failed(err);
            return;
        }

        ///登录成功，等待跳转界面和一些处理

        if(jsonObj.contains("uid") && jsonObj.contains("name") && jsonObj.contains("token")){
            UserMgr::getInstance()->SetUid(jsonObj["uid"].toInt());
            UserMgr::getInstance()->setName(jsonObj["name"].toString());
            UserMgr::getInstance()->setToken(jsonObj["token"].toString());
            UserMgr::getInstance()->setSex(jsonObj["sex"].toInt());
            UserMgr::getInstance()->setIcon(jsonObj["icon"].toString());
            UserMgr::getInstance()->setEmail(jsonObj["email"].toString());
            UserMgr::getInstance()->setDesc(jsonDoc["desc"].toString());


            emit sig_switch_chat();
            ////可能需要携带一些信息给到 mainwindow ，例如 名字、
            qDebug()<<"等待跳转至聊天界面...";

        }else{
            qDebug()<<"jsonObj 字段无法解析...";

            emit sig_login_failed(err);
        }

    });

    /// 注册网络查找用户请求回包
    _handlers.insert(ReqId::ID_SEARCH_USER_RSP, [this](ReqId id,int len,QByteArray data){
        Q_UNUSED(len);
        Q_UNUSED(id);
        ///qDebug()<<"handle id is "<<id <<"  data is "<<data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if(jsonDoc.isNull()){
            qDebug()<<"failed to create QJsonDocument...";
            emit sig_user_search_failed(0);
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if(!jsonObj.contains("error")){
            int err = ErrorCodes::ERR_JSON;
            qDebug()<<"Json callBack Errors: not contain {error}";
            emit sig_user_search_failed(err);            ////发送登陆失败的信号
            return;
        }
        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug()<<"cannot search the user,the error is : "<<err;
            emit sig_user_search_failed(err);
            return;
        }

        auto q_uid = std::make_shared<SearchInfo>(jsonObj["uid"].toInt(),jsonObj["name"].toString(),
                        jsonObj["nick"].toString(),jsonObj["desc"].toString(),
                                                  jsonObj["sex"].toInt(),jsonObj["icon"].toString());

        emit sig_user_search_finish(q_uid);

    });

    /// 注册 申请添加好友 请求回包
    _handlers.insert(ReqId::ID_ADD_FRIEND_RSP, [this](ReqId id,int len,QByteArray data){
        Q_UNUSED(len);
        Q_UNUSED(id);
        ///qDebug()<<"handle id is "<<id <<"  data is "<<data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if(jsonDoc.isNull()){
            qDebug()<<"failed to create QJsonDocument...";
            emit sig_apply_friend_send_result(100);
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if(!jsonObj.contains("error")){
            int err = ErrorCodes::ERR_JSON;
            qDebug()<<"Json callBack Errors: not contain {error}";
            emit sig_apply_friend_send_result(err);            ////发送登陆失败的信号
            return;
        }
        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug()<<"ErrorCodes is: "<<err<<", isnot the SUCCESS";
            emit sig_apply_friend_send_result(err);
            return;
        }

        // 错误码正确，发送正确回包 ,err = sucess(0)
        emit sig_apply_friend_send_result(err);

    });

    /// 收到服务器发来的 好友请求 的通知
    _handlers.insert(ReqId::ID_NOTIFY_ADD_FRIEND_REQ, [this](ReqId id,int len,QByteArray data){
        Q_UNUSED(len);
        Q_UNUSED(id);
        ///qDebug()<<"ID_NOTIFY_ADD_FRIEND_REQ handle id is "<<id ;
        ///qDebug()<<"ID_NOTIFY_ADD_FRIEND_REQ data is :"<<data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if(jsonDoc.isNull()){
            qDebug()<<"failed to create QJsonDocument...";
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if(!jsonObj.contains("error")){
            qDebug()<<"Json callBack Errors: not contain {error}";
            return;
        }
        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug()<<"ErrorCodes is: "<<err<<", isnot the SUCCESS";
            return;
        }

        // 错误码正确，发送正确回包 ,err = sucess(0)
        FriendRequest item;
        item.uid = jsonObj["applyuid"].toInt();
        item.name = jsonObj["name"].toString();
        item.message = jsonObj["desc"].toString();     /// 这个是请求的留言，并不是用户的签名 ，服务器需要修改
        item.avatar = jsonObj["icon"].toString();
        item.nick = jsonObj["nick"].toString();
        item.email = jsonObj["email"].toString();
        item.desc = jsonObj["saysay"].toString();   /// 暂时让服务器签名的字段为 saysay
        item.sex = jsonObj["sex"].toInt();
        item.accepted = false;

        emit this->sig_new_friend_apply_comming(std::make_shared<FriendRequest>(item));


    });

    /// 同意对方好友申请 回包（对方发来的好友请求，我同意）
    _handlers.insert(ReqId::ID_AUTH_FRIEND_RSP,[this](ReqId id,int len ,QByteArray data){
        Q_UNUSED(len);
        Q_UNUSED(id);
        ///qDebug()<<"ID_AUTH_FRIEND_RSP handle id is "<<id ;
        ///qDebug()<<"ID_AUTH_FRIEND_RSP data is :"<<data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if(jsonDoc.isNull()){
            qDebug()<<"failed to create QJsonDocument...";
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if(!jsonObj.contains("error")){
            qDebug()<<"Json callBack Errors: not contain {error}";
            return;
        }
        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug()<<"ErrorCodes is: "<<err<<", isnot the SUCCESS";
            return;
        }

        FriendItem item;
        item.uid = jsonObj["uid"].toInt();
        item.name = jsonObj["name"].toString();
        item.email = jsonObj["email"].toString();
        item.nick = jsonObj["nick"].toString();
        item.desc = jsonObj["desc"].toString();
        item.sex = jsonObj["sex"].toInt();
        item.avatar = jsonObj["icon"].toString();
        item.isTopItem = false;

        emit this->sig_ID_AUTH_FRIEND_RSP_callback(std::make_shared<FriendItem>(item));
        /// 将该好友添加到好友列表中

    });

    /// 我给对方发送的好友申请，对方同意了的 GRPC 回调
    _handlers.insert(ReqId::ID_NOTIFY_AUTH_FRIEND_REQ,[this](ReqId id,int len ,QByteArray data){
        Q_UNUSED(len);
        Q_UNUSED(id);
        ///qDebug()<<"ID_NOTIFY_AUTH_FRIEND_REQ handle id is "<<id ;
        ///qDebug()<<"ID_NOTIFY_AUTH_FRIEND_REQ data is :"<<data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if(jsonDoc.isNull()){
            qDebug()<<"failed to create QJsonDocument...";
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if(!jsonObj.contains("error")){
            qDebug()<<"Json callBack Errors: not contain {error}";
            return;
        }
        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug()<<"ErrorCodes is: "<<err<<", isnot the SUCCESS";
            return;
        }

        FriendItem item;
        item.uid = jsonObj["uid"].toInt();
        item.name = jsonObj["name"].toString();
        item.email = jsonObj["email"].toString();
        item.nick = jsonObj["nick"].toString();
        item.desc = jsonObj["desc"].toString();
        item.sex = jsonObj["sex"].toInt();
        item.avatar = jsonObj["icon"].toString();
        item.isTopItem = false;

        emit this->sig_ID_NOTIFY_AUTH_FRIEND_REQ_callback(std::make_shared<FriendItem>(item));

    });

    /// 我给对方发送消息
    _handlers.insert(ReqId::ID_TEXT_CHAT_MSG_RSP,[this](ReqId id,int len ,QByteArray data){
        Q_UNUSED(len);
        Q_UNUSED(id);
        ///qDebug()<<"ID_TEXT_CHAT_MSG_RSP handle id is "<<id ;
        ///qDebug()<<"ID_TEXT_CHAT_MSG_RSP data is :"<<data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if(jsonDoc.isNull()){
            qDebug()<<"failed to create QJsonDocument...";
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if(!jsonObj.contains("error")){
            qDebug()<<"Json callBack Errors: not contain {error}";
            return;
        }
        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug()<<"ErrorCodes is: "<<err<<", isnot the SUCCESS";
            return;
        }

        auto useruid = jsonObj["touid"].toInt();
        auto marking = jsonObj["marking"].toString();

        emit this->sig_uodateMessage(useruid, marking);

    });

    /// 对方给我发送消息的 GRPC 回调函数
    _handlers.insert(ReqId::ID_NOTIFY_TEXT_CHAT_MSG_REQ,[this](ReqId id,int len ,QByteArray data){
        Q_UNUSED(len);
        Q_UNUSED(id);
        ///qDebug()<<"ID_NOTIFY_TEXT_CHAT_MSG_REQ handle id is "<<id ;
        ///qDebug()<<"ID_NOTIFY_TEXT_CHAT_MSG_REQ data is :"<<data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if(jsonDoc.isNull()){
            qDebug()<<"failed to create QJsonDocument...";
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if(!jsonObj.contains("error")){
            qDebug()<<"Json callBack Errors: not contain {error}";
            return;
        }
        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug()<<"ErrorCodes is: "<<err<<", isnot the SUCCESS";
            return;
        }

        auto selfuid = jsonObj["selfuid"].toInt();
        auto touid = jsonObj["touid"].toInt();
        auto marking = jsonObj["marking"].toString();
        auto message = jsonObj["message"].toString();
        auto type = jsonObj["type"].toInt();
        qDebug()<<"type is : "<<type;

        emit this->sig_notify_updateMessage(selfuid,touid,marking,message,type);

    });

    /// 通过 uid 获取好友列表
    _handlers.insert(ReqId::ID_GET_FRIEND_LIST_FROM_UID_RSP,[this](ReqId id,int len ,QByteArray data){
        Q_UNUSED(len);
        Q_UNUSED(id);
        ///qDebug()<<"ID_GET_FRIEND_LIST_FROM_UID_RSP handle id is "<<id ;
        ///qDebug()<<"ID_GET_FRIEND_LIST_FROM_UID_RSP data is :"<<data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if(jsonDoc.isNull()){
            qDebug()<<"failed to create QJsonDocument...";
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if(!jsonObj.contains("error")){
            qDebug()<<"Json callBack Errors: not contain {error}";
            return;
        }
        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug()<<"ErrorCodes is: "<<err<<", isnot the SUCCESS";
            return;
        }
        QList<FriendItem> list;
        if (jsonObj.contains("friends") && jsonObj["friends"].isArray()) {
            QJsonArray friendsArray = jsonObj["friends"].toArray();
            for (const QJsonValue& val : friendsArray) {
                QJsonObject friendObj = val.toObject();
                FriendItem item;
                item.uid = friendObj.value("uid").toInt();
                item.name = friendObj.value("name").toString();
                item.email = friendObj.value("email").toString();
                item.nick = friendObj.value("nick").toString();
                item.desc = friendObj.value("desc").toString();
                item.sex = friendObj.value("sex").toInt();
                item.avatar = friendObj.value("icon").toString();
                list.append(item);
            }
        }
        emit this->sig_getFriendListFromUid(list);

    });

    /// 通过 uid 获取申请好友列表
    _handlers.insert(ReqId::ID_GET_FRIEND_REQUESTLIST_FROM_UID_RSP,[this](ReqId id,int len ,QByteArray data){
        Q_UNUSED(len);
        Q_UNUSED(id);
        ///qDebug()<<"ID_GET_FRIEND_REQUESTLIST_FROM_UID_RSP handle id is "<<id ;
        ///qDebug()<<"ID_GET_FRIEND_REQUESTLIST_FROM_UID_RSP data is :"<<data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if(jsonDoc.isNull()){
            qDebug()<<"failed to create QJsonDocument...";
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if(!jsonObj.contains("error")){
            qDebug()<<"Json callBack Errors: not contain {error}";
            return;
        }
        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug()<<"ErrorCodes is: "<<err<<", isnot the SUCCESS";
            return;
        }
        QList<FriendRequest> list;
        if (jsonObj.contains("requests") && jsonObj["requests"].isArray()) {
            QJsonArray requestsArray = jsonObj["requests"].toArray();
            for (const QJsonValue &val : requestsArray) {
                QJsonObject requestObj = val.toObject();

                FriendRequest request;
                // 填充好友信息
                QJsonObject userInfo = requestObj["user_info"].toObject();
                request.uid = userInfo["uid"].toInt();
                request.name = userInfo["name"].toString();
                request.email = userInfo["email"].toString();
                request.nick = userInfo["nick"].toString();
                request.desc = userInfo["desc"].toString();
                request.sex = userInfo["sex"].toInt();
                request.avatar = userInfo["icon"].toString();

                // 填充申请信息
                request.message = requestObj["message"].toString();
                request.accepted = requestObj["status"].toInt() == 1; // 1 表示已接受

                list.append(request);
            }
        }
        emit this->sig_getFriendRequestListFromUid(list);

    });


    /// 通过 uid 获取聊天记录
    _handlers.insert(ReqId::ID_GET_CHAT_LIST_FROM_UID_RSP,[this](ReqId id,int len ,QByteArray data){
        Q_UNUSED(len);
        Q_UNUSED(id);
        ///qDebug()<<"--------------ID_GET_CHAT_LIST_FROM_UID_RSP handle id is "<<id ;
        ///qDebug()<<"--------------ID_GET_CHAT_LIST_FROM_UID_RSP data is :"<<data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if(jsonDoc.isNull()){
            qDebug()<<"failed to create QJsonDocument...";
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if(!jsonObj.contains("error")){
            qDebug()<<"Json callBack Errors: not contain {error}";
            return;
        }
        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug()<<"ErrorCodes is: "<<err<<", isnot the SUCCESS";
            return;
        }

        auto selfuid = jsonObj["selfuid"].toInt();
        auto useruid = jsonObj["useruid"].toInt();
        auto index = jsonObj["index"].toInt();
        if(selfuid != UserMgr::getInstance()->getUid()) return;

        /// message?
        if (!jsonObj.contains("message") || !jsonObj["message"].isArray()) {
            qDebug() << "Json callback does not contain valid 'message' array";
            return;
        }
        QList<MessageItem> list;
        QJsonArray messageArray = jsonObj["message"].toArray();

        for (const QJsonValue& value : messageArray) {
            if (!value.isObject()) continue;

            MessageItem it;
            QJsonObject msgObj = value.toObject();

            it.uid = msgObj["sender_id"].toInt();
            it.marking = msgObj["marking"].toString();
            it.senderId = "";
            it.text = msgObj["content"].toString();
            it.avatar = "";
            it.timestamp = QDateTime::fromString("2025-01-01 00:00:00", "yyyy-MM-dd HH:mm:ss");
            it.status = MessageStatus::Sent;
            if(msgObj["msg_type"].toInt() == 1){
                it.type = MessageType::NormalMessage;
            }else if(msgObj["msg_type"].toInt() == 2){
                it.type = MessageType::PixMap;
            }else if(msgObj["msg_type"].toInt() == 3){
                it.type = MessageType::File;
            }


            list.append(it);
        }
        emit sig_message_data_to_update(useruid,index,list);

    });


    /// 通过 uid 获取更多的聊天记录
    _handlers.insert(ReqId::ID_LOADING_MESSAGE_FROM_UID_RSP,[this](ReqId id,int len ,QByteArray data){
        Q_UNUSED(len);
        Q_UNUSED(id);
        ///qDebug()<<"------------------ID_LOADING_MESSAGE_FROM_UID_RSP handle id is "<<id ;
        ///qDebug()<<"------------------ID_LOADING_MESSAGE_FROM_UID_RSP data is :"<<data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if(jsonDoc.isNull()){
            qDebug()<<"failed to create QJsonDocument...";
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if(!jsonObj.contains("error")){
            qDebug()<<"Json callBack Errors: not contain {error}";
            return;
        }
        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug()<<"ErrorCodes is: "<<err<<", isnot the SUCCESS";
            return;
        }

        /// todo...
        auto selfuid = jsonObj["selfuid"].toInt();
        auto touid = jsonObj["useruid"].toInt();
        auto messMarking = jsonObj["messMarking"].toString();
        auto loadMarking = jsonObj["loadMarking"].toString();

        if (selfuid != UserMgr::getInstance()->getUid()) return;

        if (!jsonObj.contains("message") || !jsonObj["message"].isArray()) {
            qDebug() << "Json callback does not contain valid 'message' array";
            return;
        }

        QList<MessageItem> list;
        QJsonArray messageArray = jsonObj["message"].toArray();

        for (const QJsonValue& value : messageArray) {
            if (!value.isObject()) continue;

            QJsonObject msgObj = value.toObject();
            MessageItem it;

            it.uid = msgObj["sender_id"].toInt();
            it.marking = msgObj["marking"].toString();
            it.senderId = "";
            it.text = msgObj["content"].toString();
            it.avatar = "";
            it.timestamp = QDateTime::fromString("2025-01-01 00:00:00", "yyyy-MM-dd HH:mm:ss"); // 可替换为真实时间字段
            it.status = MessageStatus::Sent;
            if(msgObj["msg_type"].toInt() == 1){
                it.type = MessageType::NormalMessage;
            }else if(msgObj["msg_type"].toInt() == 2){
                it.type = MessageType::PixMap;
            }else if(msgObj["msg_type"].toInt() == 3){
                it.type = MessageType::File;
            }

            list.append(it);
        }

        /// emit 信号：加载更多聊天记录，插入模型前部（顶部）
        emit sig_message_data_to_prepend(touid, messMarking, loadMarking , list);


    });


    /// 获取 fileserver 接口地址回包
    _handlers.insert(ReqId::ID_GET_FILESERVER_PATH_RSP, [this](ReqId id,int len,QByteArray data){
        Q_UNUSED(len);
        Q_UNUSED(id);
        ///qDebug()<<"handle id is "<<id <<"  data is "<<data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if(jsonDoc.isNull()){
            qDebug()<<"failed to create QJsonDocument...";
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if(!jsonObj.contains("error")){
            qDebug()<<"Json callBack Errors: not contain {error}";
            return;
        }
        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug()<<"ErrorCodes is: "<<err<<", isnot the SUCCESS";
            return;
        }

        // 错误码正确，发送正确回包 ,err = sucess(0)
        auto host = jsonObj["fileHost"].toString();
        auto port = jsonObj["filePort"].toString();
        auto is_send = jsonObj["is_send"].toInt();

        emit sig_getFileServerInfo(host,port, is_send);
    });


    /// 异地登陆，通知下线
    _handlers.insert(ReqId::ID_NOTIFY_OFF_LINE_REQ, [this](ReqId id,int len,QByteArray data){
        Q_UNUSED(len);
        Q_UNUSED(id);
        ///qDebug()<<"handle id is "<<id <<"  data is "<<data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if(jsonDoc.isNull()){
            qDebug()<<"failed to create QJsonDocument...";
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if(!jsonObj.contains("error")){
            qDebug()<<"Json callBack Errors: not contain {error}";
            return;
        }
        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug()<<"ErrorCodes is: "<<err<<", isnot the SUCCESS";
            return;
        }

        // 错误码正确，发送正确回包 ,err = sucess(0)
        auto uid = jsonObj["uid"].toInt();

        /// 设置 socket 断开状态
        _socket_off_status = socketOffStatus::CHANGE_LOG_OFF_SOCKET;

        emit NotifyOffline(uid);
    });

    /// 心跳包
    _handlers.insert(ReqId::ID_HEART_BEAT_MESSAGE_RSP, [](ReqId id,int len,QByteArray data){
        Q_UNUSED(len);
        Q_UNUSED(id);
        ///qDebug()<<"handle id is "<<id <<"  data is "<<data;

        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        if(jsonDoc.isNull()){
            qDebug()<<"failed to create QJsonDocument...";
            return;
        }
        QJsonObject jsonObj = jsonDoc.object();
        if(!jsonObj.contains("error")){
            qDebug()<<"Json callBack Errors: not contain {error}";
            return;
        }
        int err = jsonObj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            qDebug()<<"ErrorCodes is: "<<err<<", isnot the SUCCESS";
            return;
        }

        // 错误码正确，发送正确回包 ,err = sucess(0)
        auto uid = jsonObj["uid"].toInt();
        qDebug()<<"心跳包发送成功，uid is: "<<uid<<",    time is: "<<QTime::currentTime();
    });
}

QTcpSocket* TcpMgr::getSocket()
{
    return _socket;
}























