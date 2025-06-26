#ifndef GLOBAL_H
#define GLOBAL_H

#include <QWidget>
#include <functional>
#include <QRegularExpression>
#include "QStyle"
#include <QString>
#include <QMap>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDir>
#include <QSettings>
#include <QByteArray>
#include <QRandomGenerator>
#include <QList>

template<typename T>
T* createNamedWidget(const QString& name, QWidget* parent = nullptr) {
    T* widget = new T(parent);
    widget->setObjectName(name);
    return widget;
}

QString formatFileSize(qint64 size);

QString getFileIconFromSuffix(QString suffix);

///生成指定长度范围内的随机字符串
QString generateRandomString(int minLength, int maxLength,
            const QString &charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");

///用来刷新 QWidget 的样式
extern std::function<void(QWidget*)> repolish;
///加密
extern std::function<QString(const QString& input,const QString& key,const QString& salt)> enhancedXorEncrypt;
///解密
extern std::function<QString(const QString& baseInput,const QString& key,int saltLength)> enhancedXorDecrypt;
extern QString key;                //加密密钥
extern QString salt;               //扰动因子

extern QString gate_url_prefix;

#define WINDOWSWIDTHINIT    900
#define WINDOWSHEIGHTINIT   620
#define WINDOWSMOVE         20000

#define MAX_FILE_LEN    8192
#define TCP_HEAD_LEN    6

//请求 id
enum ReqId{
    ID_GET_VARIFY_CODE = 1001,   //获取验证码
    ID_REG_USER = 1002,          //注册用户
    ID_RESET_PED = 1003,         //重置密码
    ID_LOGIN_USER = 1004,        //用户登录
    ID_CHAT_LOGIN = 1005,        //登录聊天服务器
    ID_CHAT_LOGIN_RSP = 1006,    //登录聊天服务器会包
    ID_SEARCH_USER_REQ = 1007, //用户搜索请求
    ID_SEARCH_USER_RSP = 1008, //搜索用户回包
    ID_ADD_FRIEND_REQ = 1009,  //添加好友申请
    ID_ADD_FRIEND_RSP = 1010, //申请添加好友回复
    ID_NOTIFY_ADD_FRIEND_REQ = 1011,  //通知用户添加好友申请
    ID_AUTH_FRIEND_REQ = 1013,  //认证好友请求
    ID_AUTH_FRIEND_RSP = 1014,  //认证好友回复
    ID_NOTIFY_AUTH_FRIEND_REQ = 1015, //通知用户认证好友申请
    ID_TEXT_CHAT_MSG_REQ  = 1017,  //文本聊天信息请求
    ID_TEXT_CHAT_MSG_RSP  = 1018,  //文本聊天信息回复
    ID_NOTIFY_TEXT_CHAT_MSG_REQ = 1019, //通知用户文本聊天信息

    ID_GET_FRIEND_LIST_FROM_UID_REQ = 1020,             /// 加载好友列表
    ID_GET_FRIEND_LIST_FROM_UID_RSP = 1021,

    ID_GET_FRIEND_REQUESTLIST_FROM_UID_REQ = 1022,      /// 加载好友请求列表
    ID_GET_FRIEND_REQUESTLIST_FROM_UID_RSP = 1023,

    ID_GET_CHAT_LIST_FROM_UID_REQ = 1024,               /// 加载聊天记录
    ID_GET_CHAT_LIST_FROM_UID_RSP = 1025,

    ID_LOADING_MESSAGE_FROM_UID_REQ = 1026,             /// 加载更多聊天记录
    ID_LOADING_MESSAGE_FROM_UID_RSP = 1027,

    ID_GET_FILESERVER_PATH_REQ = 1028,                  /// 获取 fileserver 接口地址
    ID_GET_FILESERVER_PATH_RSP = 1029,

    ID_NOTIFY_OFF_LINE_REQ = 1030,        /// 异地登录，通知下线


    ID_UPLOAD_FILE_REQ = 10001,                          /// 分片上传文件
    ID_UPLOAD_FILE_RSP = 10002,

    iD_GET_FILE_FROM_MARKING_REQ = 10003,
    iD_GET_FILE_FROM_MARKING_RSP = 10004,

    ID_HEART_BEAT_MESSAGE_REQ = 1050,
    ID_HEART_BEAT_MESSAGE_RSP = 1051,

};

//模块
enum Modules{
    REGISRERMOD = 0,      //注册模块
    FINDPASSWOEDMOD = 1,  //找回密码模块
    LOGMOD = 2,           //登录模块
};

//错误码
enum ErrorCodes{
    SUCCESS = 0,
    ERR_JSON = 1,       //json 解析错误
    ERR_NETWORK = 2,    //网络错误
};

// 聊天服务器信息
struct ServerInfo{
    QString Host;
    QString Port;
    QString Token;
    int Uid;
};










// ------------------------------------------聊天界面--------------聊天记录信息--------------------------------
enum MessageStatus {
    Sending = 1001,    /// 正在发送
    Sent    = 1002,    /// 发送成功
    Failed  = 1003,    /// 发送失败
};
enum MessageType{
    NormalMessage = 1011,    /// 普通消息
    TimeItem      = 1012,    /// 时间提示
    LoadMessage   = 1013,    /// 加载更多的记录动画
    File          = 1014,
    PixMap        = 1015,
};

struct MessageItem
{
    int uid;                /// 对方uid
    QString marking;        /// 该条消息的唯一标识
    QString senderId;       /// 发送者ID : name or self
    QString text;           /// 文本内容
    QString avatar;         /// 头像
    QDateTime timestamp;    /// 时间戳
    MessageStatus status;   /// 对文本： sending , sent , failed
    MessageType type;       /// 消息类型 : NormalMessage , TimeItem
};

// ------------------------------------------聊天界面--------------用户列表信息-------------------------------
struct chatUserListItem{
    QString name;
    QString avatar;           ///头像
    QString time;
    QString message;          ///最后一条消息内容

    int uid;
    QList<MessageItem> chatdata;

};













// ------------------------------------------好友界面---------------好友列表信息--------------------------------
struct FriendItem {
    int uid;               /// 用户 id
    QString name;          /// 用户昵称
    QString email;         /// 用户邮箱
    QString nick;          /// 用户备注
    QString desc;          /// 用户签名
    int sex;               /// 用户性别
    QString avatar;        /// 用户头像

    bool isTopItem = false;  /// 是否为第一个固定项（第一项为新朋友列表，之后才为好友列表）
};

// ------------------------------------------好友界面---------------好友申请列表信息-----------------------------
struct FriendRequest {
    int uid;               /// 用户 id
    QString name;          /// 用户昵称
    QString email;         /// 用户邮箱
    QString nick;          /// 用户备注
    QString desc;          /// 用户签名
    int sex;               /// 用户性别
    QString avatar;        /// 用户头像

    QString message;       /// 申请留言
    bool accepted = false;
};

// ------------------------------------------好友界面----------------搜索用户信息-----------------------------
class SearchInfo{
public:
    int _uid;
    QString _name;
    QString _nick;
    QString _desc;
    int _sex;
    QString _icon;
    SearchInfo(int uid,QString name,QString nick,QString desc,int sex,QString icon);
    SearchInfo();
};


























#endif // GLOBAL_H
