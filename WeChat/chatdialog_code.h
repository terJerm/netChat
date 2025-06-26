#ifndef CHATDIALOG_CODE_H
#define CHATDIALOG_CODE_H

#include <QDialog>
#include "chatuserlistview.h"
#include <chattextedit.h>
#include "sendfiledialog.h"
#include <QTimer>



class QPoint;
class QMouseEvent;
class QListView;
class QTextEdit;
class QLineEdit;
class QLabel;
class ChatUserListModel;
class chatUserListDelegate;
class chatMessageListMOdel;
class chatMessageListDelegate;
class chatMessageListView;
class QSplitter;
class QStackedWidget;
class chatFriendListView;
class chatFriendListModel;
class chatFriendListDelegate;
class FriendRequestDelegate;
class FriendRequestModel;
class friendInfoWidget;
class applyFriendWId;
class SearchInfo;
struct FriendRequest;
struct FriendItem;

namespace Ui {
class chatDialog_code;
}

enum searchLineEditStatus{
    SEARCHCHAT,
    SEARCHFRIENDFROMLIST,
    SEARCHFRIENDFROMSERVE,

};

/**************************************************************/

//   * @file:      chatdialog_code.h 登录后的通信主界面， 调度所有的子界面及模块的信号交互

//   * main_stack: 2个子界面，(聊天/朋友 左侧+右侧) …………(文件)
//       * stack_l: 2个子界面，(聊天列表，朋友列表)
//       * stack_r: 4个子界面，(聊天记录，好友请求列表，好友信息界面，申请好友界面)

//   * @date:      2025/05/06

/**************************************************************/

class chatDialog_code : public QDialog
{
    Q_OBJECT

public:
    explicit chatDialog_code(QWidget *parent = nullptr);
    ~chatDialog_code();

    void initMainChatWid();

private:
    void mousePressEvent(QMouseEvent* event) override;     /// 三个虚函数实现鼠标拖动窗口功能
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

    void initUI();                /// 初始化主界面的 UI
    void readQss();               /// 加载所需的 qss 文件
    void updateSideBtn(QPushButton*,int index);    /// 更新侧边栏按钮的状态

    void sendMessageSlots(QString message);         ///连接mytextEdit的发送信号，的槽函数

    void getFriendListMessage(int uid);             /// 通过 uid 获取用户好友列表
    void getFriendRequestListMessage(int uid);      /// 通过本次登录用户的 uid 从服务器获取 对应的好友请求列表

    void chatBtnClicked();    ///侧边栏聊天按钮槽函数
    void friendBtnClicked();  ///侧边栏好友按钮槽函数
    void addBtnSlots();       ///搜索框右侧的按钮槽函数


private slots:
    void changeToFriendRequestViewSlot();
    void changeToFriendInfoWid(std::shared_ptr<SearchInfo>);
    void lineEidtChangeFinish();
    void lineEidtChange(const QString &text);
    void _friend_info_wid_addFriendSlo();       /// 朋友信息界面的添加好友按钮按下的槽函数
    void applyFriendWidNoBtnSlots();            /// 申请朋友界面取消按钮的槽函数
    void slot_send_btn_clicked();
    void slot_apply_friend_send_result(int err);  ///添加好友回包发射的槽函数

    void slot_user_search_finish(std::shared_ptr<SearchInfo>);  /// 网络查找用户成功槽函数
    void slot_user_search_failed(int e);                        /// 网络查找用户失败槽函数
    void tcp_slot_new_friend_apply_comming(std::shared_ptr<FriendRequest>);  /// 收到新的好友请求发送的槽函数
    void slot_ID_AUTH_FRIEND_RSP_callback(std::shared_ptr<FriendItem>);  /// 同意对方好友请求的回包
    void slot_ID_NOTIFY_AUTH_FRIEND_REQ_callback(std::shared_ptr<FriendItem>);  ///我给对方发送的好友申请，对方同意了的 槽函数
    void slot_updateMessage(int useruid,QString marking);           /// 给对方发送消息的槽函数
    void slot_notify_updateMessage(int selfuid,int touid,QString marking,QString message,int type);  ///对方给我发送消息的处理槽函数
    void slot_userListDoubleClicked(QString name,QList<MessageItem> data,int nowuid);
    void slot_getFriendListFromUid(QList<FriendItem> list);          ///获取好友列表的槽函数
    void slot_getFriendRequestListFromUid(QList<FriendRequest>list);   /// 获取好友请求列表的槽函数
    void slot_message_data_to_update(int&,int& ,QList<MessageItem>);   /// 获取聊天记录后的槽函数
    void slot_message_data_to_prepend(int& uid,QString& messMarking,QString& loadMarking,QList<MessageItem>list); /// 获取更早的聊天记录的槽函数
    void _msg_btn_clicked_slot(int uid);
    void loadMoreChatMessage();        /// 加载更多聊天记录槽函数
    void fileDropped_slot(QString filePath);
    void slot_getFileServerInfo(QString,QString,int);  /// 获取文件服务器信息的槽函数
    void slot_fileserver_socketConnectChange(bool, int is_send);

    void slot_send_file_callback(quint16 msg_id, const QJsonObject& jsonObj);  ///发送文件服务器回包

    void addFileItems(QString& name,QString& size,QString& suffix,QString& marking,bool isself);  /// 文件发送完成后，在聊天记录中添加这个文件记录

    void slot_downloadClicked(const QModelIndex &index);    /// 下载文件槽函数
    void slot_openClicked(const QModelIndex &index);        /// 打开文件槽函数
    void NotifyOfflineSlot(int uid);
    void heartOuttimeOffSocketSlot();

signals:
    void sigChatChangeToLogWid();

private:
    Ui::chatDialog_code *ui;

    bool _mousePressed = false;
    QPoint _mousePos;                    // 鼠标按下时的全局位置
    QPoint _windowPos;                   // 鼠标按下时窗口左上角位置
    QRect _normalGeometry;               // 记录正常大小下的位置和尺寸
    QPointF _clickRatio;                 // 鼠标点击点在窗口中的相对比例（用于最大化时还原）
    const int _titleHeight = 35;         // 可拖动区域高度

    QStackedWidget* _main_stack;

    QStackedWidget* _stack_l,*_stack_r;

    QSplitter* spli;                     // 中心 stackWidget 的第一个界面 ：spli---------------------------------
    chatUserListView* _user_list_view;
    ChatUserListModel* _user_list_model;           /// 聊天列表的模型视图代理自定义实现
    chatUserListDelegate* _user_list_delegate;

    bool _request_send_panding;
    bool _is_init_friend_apply;


    chatMessageListView* _chat_list_view;
    chatMessageListDelegate* _chat_list_delegate;   /// 聊天记录列表的模型视图代理自定义实现
    chatMessageListMOdel* _chat_list_model;
    ChatTextEdit* _text_edit;
    int _now_uid;

    chatFriendListView* _friend_list_view;
    chatFriendListModel* _friend_list_model;        /// 好友列表的模型视图代理自定义实现
    chatFriendListDelegate* _friend_list_delegate;

    QListView* _friend_request_view;
    FriendRequestModel* _friend_request_model;      /// 好友请求列表的模型视图代理自定义实现
    FriendRequestDelegate* _friend_request_dalegate;


    QPushButton* _head_btn;
    QPushButton* _chat_btn;
    QPushButton* _friend_btn;
    QPushButton* _collect_btn;            /// 侧边栏按钮
    QPushButton* _file_btn;
    QPushButton* _set_btn;

    QPushButton* _emoji_btn;
    QPushButton* _open_file_btn;       /// 输入框上侧的三个按钮
    QPushButton* _send_btn;

    QPushButton* _close_btn;
    QPushButton* _max_btn;
    QPushButton* _min_btn;             /// 右上角最大化最小化按钮

    QLineEdit* _line_edit;
    QPushButton* _add_btn;
    searchLineEditStatus _search_enum;
    QLabel* _name_label;

    applyFriendWId* _apply_friend_wid;

    friendInfoWidget* _friend_info_wid;         // 中心 stackWidget 的第一个界面 ：spli---------------------------------

    sendFileDialog* _send_file_dialog;

    bool is_recv_file = false;
    QString marking;                /// 接收文件的 marking 标志，检测包
    QString recv_file_path;


};

#endif // CHATDIALOG_CODE_H
