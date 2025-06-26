#include "chatdialog_code.h"
#include "ui_chatdialog_code.h"
#include <QMouseEvent>
#include "global.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSplitter>
#include <QLabel>
#include <QStackedWidget>
#include <QListView>
#include <QTextEdit>
#include <QLineEdit>
#include "chatuserlistmodel.h"
#include "chatuserlistdelegate.h"
#include <QScroller>
#include <QVariantMap>
#include "chatmessagelistdelegate.h"
#include "chatmessagelistmodel.h"
#include "chatmessagelistview.h"
#include <QDateTime>
#include <QScrollerProperties>
#include <QEasingCurve>
#include "chatfriendlistview.h"
#include "chatfriendlistdelegate.h"
#include "chatfriendlistmodel.h"
#include "friendrequestdelegate.h"
#include "friendrequestmodel.h"
#include "friendinfowidget.h"
#include "applyfriendwid.h"
///#include "stackedwidgetanimator.h"
#include "tcpmgr.h"
#include "tcpfilemgr.h"
#include "usermgr.h"
#include <QUuid>
#include <QPropertyAnimation>
#include <QScrollBar>
#include <QThread>
#include <QMessageBox>
#include "mytipwidget.h"
#include <QFileInfo>
#include <QProcess>
#include "customdialog.h"

chatDialog_code::chatDialog_code(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::chatDialog_code),_request_send_panding(false),_is_init_friend_apply(false),_now_uid(0),_search_enum(SEARCHCHAT)
{
    ui->setupUi(this);

    this->setWindowFlags(Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_QuitOnClose, false);       ///设置为非主窗口
    this->resize(WINDOWSWIDTHINIT,WINDOWSHEIGHTINIT);

    this->initUI();
    _user_list_model = new ChatUserListModel(this);
    _user_list_delegate = new chatUserListDelegate(this);
    _user_list_view->setModel(_user_list_model);
    _user_list_view->setItemDelegate(_user_list_delegate);
    _user_list_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);    ///隐藏水平滚动条


    _chat_list_delegate = new chatMessageListDelegate(this);
    _chat_list_model = new chatMessageListMOdel(this);
    _chat_list_view->setModel(_chat_list_model);
    _chat_list_view->setItemDelegate(_chat_list_delegate);
    _chat_list_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);    ///隐藏水平滚动条


    _friend_list_delegate = new chatFriendListDelegate(this);
    _friend_list_model = new chatFriendListModel(this);
    _friend_list_view->setModel(_friend_list_model);
    _friend_list_view->setItemDelegate(_friend_list_delegate);
    _friend_list_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);    ///隐藏水平滚动条


    _friend_request_dalegate = new FriendRequestDelegate(this);
    _friend_request_model = new FriendRequestModel(this);
    _friend_request_view->setModel(_friend_request_model);
    _friend_request_view->setItemDelegate(_friend_request_dalegate);
    _friend_request_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);    ///隐藏水平滚动条


    connect(_chat_list_delegate, &chatMessageListDelegate::needUpdate, _chat_list_view->viewport(), QOverload<>::of(&QWidget::update));
    connect(_close_btn,&QPushButton::clicked,[](){QApplication::quit();});
    connect(_max_btn,&QPushButton::clicked,[this](){if (isMaximized())showNormal();else this->showMaximized();});
    connect(_min_btn,&QPushButton::clicked,[this](){this->showMinimized();});
    connect(_text_edit,&ChatTextEdit::sendMessageSignals,this,&chatDialog_code::sendMessageSlots);
    connect(_text_edit,&ChatTextEdit::fileDropped,this,&chatDialog_code::fileDropped_slot);
    connect(_friend_btn,&QPushButton::clicked,this,&chatDialog_code::friendBtnClicked);
    connect(_chat_btn,&QPushButton::clicked,this,&chatDialog_code::chatBtnClicked);
    connect(_friend_list_view,&chatFriendListView::changeToFriendRequestView,this,&chatDialog_code::changeToFriendRequestViewSlot);
    connect(_friend_list_view,&chatFriendListView::changeToFriendInfoWid,this,&chatDialog_code::changeToFriendInfoWid);
    connect(_add_btn,&QPushButton::clicked,this,&chatDialog_code::addBtnSlots);
    connect(_line_edit,&QLineEdit::editingFinished,this,&chatDialog_code::lineEidtChangeFinish);
    connect(_line_edit,&QLineEdit::textEdited,this,&chatDialog_code::lineEidtChange);
    connect(_friend_info_wid,&friendInfoWidget::_apply_friend_wid_signals,this,&chatDialog_code::_friend_info_wid_addFriendSlo);
    connect(_apply_friend_wid,&applyFriendWId::noBtnClickedSignals,this,&chatDialog_code::applyFriendWidNoBtnSlots);
    connect(_send_btn,&QPushButton::clicked,this,&chatDialog_code::slot_send_btn_clicked);
    connect(_user_list_view,&chatUserListView::userListDoubleClicked,this,&chatDialog_code::slot_userListDoubleClicked);
    connect(_friend_info_wid, &friendInfoWidget::_msg_btn_clicked_signals,this,&chatDialog_code::_msg_btn_clicked_slot);
    connect(_chat_list_model, &chatMessageListMOdel::modelResetFinished, this, [=]() {
        QTimer::singleShot(0, _chat_list_view, SLOT(scrollToBottom()));
    });
    connect(_chat_list_delegate,&chatMessageListDelegate::downloadClicked,this,&chatDialog_code::slot_downloadClicked);
    connect(_chat_list_delegate,&chatMessageListDelegate::openClicked,this,&chatDialog_code::slot_openClicked);

    connect(_chat_list_view,&chatMessageListView::overScrollTopTriggered,this,&chatDialog_code::loadMoreChatMessage);

    connect(TcpMgr::getInstance().get(),&TcpMgr::sig_user_search_failed,this,&chatDialog_code::slot_user_search_failed);
    connect(TcpMgr::getInstance().get(),&TcpMgr::sig_user_search_finish,this,&chatDialog_code::slot_user_search_finish);
    connect(TcpMgr::getInstance().get(),&TcpMgr::sig_new_friend_apply_comming,this,&chatDialog_code::tcp_slot_new_friend_apply_comming);
    connect(TcpMgr::getInstance().get(),&TcpMgr::sig_ID_AUTH_FRIEND_RSP_callback,this,&chatDialog_code::slot_ID_AUTH_FRIEND_RSP_callback);
    connect(TcpMgr::getInstance().get(),&TcpMgr::sig_ID_NOTIFY_AUTH_FRIEND_REQ_callback,this,&chatDialog_code::slot_ID_NOTIFY_AUTH_FRIEND_REQ_callback);
    connect(TcpMgr::getInstance().get(),&TcpMgr::sig_uodateMessage,this,&chatDialog_code::slot_updateMessage);
    connect(TcpMgr::getInstance().get(),&TcpMgr::sig_notify_updateMessage,this,&chatDialog_code::slot_notify_updateMessage);
    connect(TcpMgr::getInstance().get(),&TcpMgr::sig_getFriendListFromUid,this,&chatDialog_code::slot_getFriendListFromUid);
    connect(TcpMgr::getInstance().get(),&TcpMgr::sig_getFriendRequestListFromUid,this,&chatDialog_code::slot_getFriendRequestListFromUid);
    connect(TcpMgr::getInstance().get(),&TcpMgr::sig_message_data_to_update,this,&chatDialog_code::slot_message_data_to_update);
    connect(TcpMgr::getInstance().get(),&TcpMgr::sig_message_data_to_prepend,this,&chatDialog_code::slot_message_data_to_prepend);
    connect(TcpMgr::getInstance().get(),&TcpMgr::sig_apply_friend_send_result,this,&chatDialog_code::slot_apply_friend_send_result);
    connect(TcpMgr::getInstance().get(),&TcpMgr::sig_getFileServerInfo,this,&chatDialog_code::slot_getFileServerInfo);
    connect(TcpMgr::getInstance().get(),&TcpMgr::NotifyOffline,this,&chatDialog_code::NotifyOfflineSlot);
    connect(TcpMgr::getInstance().get(),&TcpMgr::heartOuttimeOffSocket,this,&chatDialog_code::heartOuttimeOffSocketSlot);

    connect(tcpFilemgr::getInstance().get(),&tcpFilemgr::socketConnectChange,this,&chatDialog_code::slot_fileserver_socketConnectChange);
    connect(tcpFilemgr::getInstance().get(),&tcpFilemgr::sig_logic_process,this,&chatDialog_code::slot_send_file_callback);

    this->readQss();

    initMainChatWid();     /// 加载用户相关数据
}

chatDialog_code::~chatDialog_code()
{
    delete ui;
}

void chatDialog_code::mousePressEvent(QMouseEvent *event){
    if (event->button() == Qt::LeftButton && event->pos().y() <= _titleHeight) {
        _mousePressed = true;
        _mousePos = event->globalPosition().toPoint();
        _windowPos = this->pos();
        if (isMaximized()) {
            /// 记录点击点相对于窗口的比例
            QSize maxSize = this->size();
            QPoint clickPos = event->pos();
            _clickRatio.setX(double(clickPos.x()) / maxSize.width());
            _clickRatio.setY(double(clickPos.y()) / maxSize.height());
            /// 保存最大化前的正常位置尺寸（你也可以改成存 resizeEvent）
            _normalGeometry = QRect(WINDOWSMOVE, WINDOWSMOVE, WINDOWSWIDTHINIT, WINDOWSHEIGHTINIT);
        }
    }
}

void chatDialog_code::mouseMoveEvent(QMouseEvent *event){
    if (_mousePressed) {
        QPoint globalPos = event->globalPosition().toPoint();
        if (isMaximized()) {
            /// 使用保存的 _normalGeometry 或你自己的逻辑
            QSize normSize = _normalGeometry.size();
            QPoint offset(
                int(_clickRatio.x() * normSize.width()),
                int(_clickRatio.y() * normSize.height())
                );
            QPoint targetTopLeft = globalPos - offset;
            setGeometry(QRect(targetTopLeft, normSize));
            _mousePos = globalPos;
            _windowPos = targetTopLeft;
            /// 切换为正常窗口
            showNormal();
        }
        QPoint delta = globalPos - _mousePos;
        move(_windowPos + delta);
    }
}

void chatDialog_code::mouseReleaseEvent(QMouseEvent *event){
    Q_UNUSED(event);
    _mousePressed = false;
}

void chatDialog_code::initUI(){
    QHBoxLayout* main_lay = new QHBoxLayout();
    main_lay->setContentsMargins(0,0,0,0);
    main_lay->setSpacing(0);
    this->setLayout(main_lay);

    ///主布局包含左侧工具栏、右侧工作区
    QWidget* side_wid = new QWidget(this);
    QWidget* work_wid = new QWidget(this);
    main_lay->addWidget(side_wid);
    main_lay->addWidget(work_wid);
    side_wid->setFixedWidth(60);

    side_wid->setStyleSheet("background-color:rgb(255,255,255) ");
    work_wid->setStyleSheet("background-color:rgb(23,54,33)");

    /// side_wid 左侧工具栏
    _head_btn = createNamedWidget<QPushButton>("_head_btn",this);
    _chat_btn = createNamedWidget<QPushButton>("_chat_btn",this);
    _friend_btn = createNamedWidget<QPushButton>("_friend_btn",this);
    _collect_btn = createNamedWidget<QPushButton>("_collect_btn",this);
    _file_btn = createNamedWidget<QPushButton>("_file_btn",this);
    _set_btn = createNamedWidget<QPushButton>("_set_btn",this);

    QVBoxLayout* vlay_30 = new QVBoxLayout(side_wid);
    vlay_30->addWidget(_head_btn,0, Qt::AlignHCenter);
    vlay_30->addWidget(_chat_btn,0, Qt::AlignHCenter);
    vlay_30->addWidget(_friend_btn,0, Qt::AlignHCenter);
    vlay_30->addWidget(_collect_btn,0, Qt::AlignHCenter);
    vlay_30->addWidget(_file_btn,0, Qt::AlignHCenter);
    vlay_30->addStretch();
    vlay_30->addWidget(_set_btn,0, Qt::AlignHCenter);

    vlay_30->setSpacing(12);
    vlay_30->setContentsMargins(5,20,5,20);

    _head_btn->setFixedSize(40,40);_head_btn->setIconSize(QSize(40,40));
    _head_btn->setIcon(QIcon(":/res/chat_res/self_1.png"));
    _chat_btn->setFixedSize(35,35);_chat_btn->setIconSize(QSize(24,24));
    _chat_btn->setIcon(QIcon(":/res/chat_res/chat_2.png"));
    _friend_btn->setFixedSize(35,35);_friend_btn->setIconSize(QSize(24,24));
    _friend_btn->setIcon(QIcon(":/res/chat_res/friend_2.png"));
    _collect_btn->setFixedSize(35,35);_collect_btn->setIconSize(QSize(24,24));
    _collect_btn->setIcon(QIcon(":/res/chat_res/collect_2.png"));
    _file_btn->setFixedSize(35,35);_file_btn->setIconSize(QSize(24,24));
    _file_btn->setIcon(QIcon(":/res/chat_res/file_2.png"));
    _set_btn->setFixedSize(35,35);_set_btn->setIconSize(QSize(24,24));
    _set_btn->setIcon(QIcon(":/res/chat_res/set_1.png"));

    ///右侧工作区包含上部状态区和中心工作区
    QWidget* top_wid = new QWidget(this);
    QWidget* cen_wid = new QWidget(this);

    top_wid->setStyleSheet("background-color: rgb(105,175,165)");
    cen_wid->setStyleSheet("background-color:rgb(213,54,133)");

    QVBoxLayout* vlay_1 = new QVBoxLayout();
    vlay_1->setContentsMargins(0,0,0,0);
    vlay_1->setSpacing(0);
    top_wid->setFixedHeight(30);
    work_wid->setLayout(vlay_1);

    vlay_1->addWidget(top_wid);
    vlay_1->addWidget(cen_wid);

    ///top 顶部状态区
    QWidget* wd_2 = new QWidget(this);
    QHBoxLayout* ly_99 = new QHBoxLayout(top_wid);
    ly_99->addStretch();
    ly_99->addWidget(wd_2,0,Qt::AlignHCenter);
    ly_99->setContentsMargins(10,0,10,0);

    ///wid_2
    _close_btn = new QPushButton(this);
    _close_btn->setFixedSize(25,25); _close_btn->setIconSize(QSize(16,16));
    _close_btn->setIcon(QIcon(":/res/chat_res/close_1.png"));
    _max_btn = new QPushButton(this);
    _max_btn->setFixedSize(25,25); _max_btn->setIconSize(QSize(16,16));
    _max_btn->setIcon(QIcon(":/res/chat_res/max_1.png"));
    _min_btn = new QPushButton(this);
    _min_btn->setFixedSize(25,25); _min_btn->setIconSize(QSize(16,16));
    _min_btn->setIcon(QIcon(":/res/chat_res/min_1.png"));
    QHBoxLayout* ly_88 = new QHBoxLayout(wd_2);
    ly_88->addStretch();
    ly_88->addWidget(_min_btn, 0, Qt::AlignHCenter);
    ly_88->addWidget(_max_btn, 0, Qt::AlignHCenter);
    ly_88->addWidget(_close_btn, 0, Qt::AlignHCenter);

    ly_88->setContentsMargins(0,0,6,0);

    ///中心工作区是一个stackwidget
    _main_stack = new QStackedWidget(this);
    QHBoxLayout* hlay_1 = new QHBoxLayout();
    cen_wid->setLayout(hlay_1);
    hlay_1->setContentsMargins(0,0,0,0);
    hlay_1->addWidget(_main_stack);

    _main_stack->setStyleSheet("background-color:rgb(113,154,133)");

    ///stackWidget的第一个界面 : 一个QSplitter 容器
    {
        spli = new QSplitter(Qt::Horizontal);
        spli->setObjectName("spli");
        _main_stack->addWidget(spli);

        spli->setStyleSheet("background-color:rgb(213,154,233)");

        QWidget* left_1 = new QWidget(this);
        QWidget* right_11 = new QWidget(this);

        spli->addWidget(left_1);
        spli->addWidget(right_11);
        spli->setChildrenCollapsible(false);   ///设置 spli 不会自动折叠子空间
        spli->setSizes({210, 9999});           ///设置左侧当前宽度

        left_1->setStyleSheet("background-color:rgb(213,64,93)");
        right_11->setStyleSheet("background-color:rgb(255,255,255)");
        right_11->setContentsMargins(0,0,0,0);

        ///left_1 :  其中是一个 listView
        left_1->setMinimumWidth(210);
        left_1->setMaximumWidth(280);

        QVBoxLayout* hlay_2 = new QVBoxLayout();
        left_1->setLayout(hlay_2);
        QWidget* wid_111 = new QWidget(this);

        _stack_l = new QStackedWidget(this);

        _user_list_view = new chatUserListView(this);
        hlay_2->addWidget(wid_111);
        hlay_2->addWidget(_stack_l);          //// 左侧的 stack_l 栈
        hlay_2->setContentsMargins(3,0,2,8);
        hlay_2->setSpacing(0);

        /// stack_l 的第一个界面： _user_list_view
        _stack_l->addWidget(_user_list_view);
        /// stack_l 的第一个界面： _friend_list_view
        _friend_list_view = new chatFriendListView(this);
        _stack_l->addWidget(_friend_list_view);

        /// wid_111
        QHBoxLayout* lay_222 = new QHBoxLayout(wid_111);
        _line_edit = new QLineEdit(this);
        _add_btn = new QPushButton(this);
        lay_222->addWidget(_line_edit);
        lay_222->addWidget(_add_btn);
        lay_222->setContentsMargins(5,2,4,0);

        _add_btn->setFixedSize(26,26); _add_btn->setIconSize(QSize(20,20));
        _add_btn->setIcon(QIcon(":/res/chat_res/add_1.png"));

        /// 右侧是一个 stack_r ,
        QHBoxLayout* hhh = new QHBoxLayout(right_11);
        _stack_r = new QStackedWidget(this);
        hhh->addWidget(_stack_r);
        hhh->setContentsMargins(0,0,0,0);

        ///右边 stack_r 第一个界面： wddd_1
        QWidget* wddd_1 = new QWidget(this);
        _stack_r->addWidget(wddd_1);
        ///右边 stack_r 第二个界面： wddd_2
        QWidget* wddd_2 = new QWidget(this);
        _stack_r->addWidget(wddd_2);

        _friend_info_wid = new friendInfoWidget(this);
        _stack_r->addWidget(_friend_info_wid);
        ///右边stack_r第四个界面：申请好友界面
        _apply_friend_wid = new applyFriendWId(this);
        _stack_r->addWidget(_apply_friend_wid);

        QVBoxLayout* ly_78 = new QVBoxLayout(wddd_1);
        QWidget* wid_io = new QWidget(this);

        QSplitter* right_1 = new QSplitter(Qt::Vertical);
        _chat_list_view = new chatMessageListView(this);

        ly_78->addWidget(wid_io);
        ly_78->addWidget(right_1);
        ly_78->setContentsMargins(0,0,0,0);
        ly_78->setSpacing(0);
        wid_io->setFixedHeight(26);
        _chat_list_view->setStyleSheet("background-color:rgb(223,214,223)");

        QWidget* wid_10 = new QWidget(this);
        QWidget* wid_11 = new QWidget(this);
        right_1->addWidget(wid_10);
        right_1->addWidget(wid_11);
        right_1->setChildrenCollapsible(false);   ///设置 spli 不会自动折叠子空间

        /// wid_io 设置： 显示用户名字
        QHBoxLayout* ly_io = new QHBoxLayout(wid_io);
        _name_label = new QLabel("whats the name",this);
        ly_io->addWidget(_name_label,0,Qt::AlignCenter);
        ly_io->setContentsMargins(0,0,0,0);

        wid_11->setMinimumHeight(180);
        wid_11->setMaximumHeight(280);

        wid_11->setStyleSheet("background-color:rgb(113,214,173)");
        right_1->setSizes({9999,180});

        ///wid_11 上面一个wid ,下面一个text Edit
        _text_edit = new ChatTextEdit(this);
        QWidget* wid_22 = new QWidget(this);

        QVBoxLayout* vlay_11 = new QVBoxLayout();
        wid_11->setLayout(vlay_11);
        vlay_11->addWidget(wid_22);
        vlay_11->addWidget(_text_edit);

        ///wid_22
        wid_22->setFixedHeight(30);
        _emoji_btn = new QPushButton(this);
        _emoji_btn->setFixedSize(26,26);_emoji_btn->setIconSize(QSize(16,16));
        _emoji_btn->setIcon(QIcon(":/res/chat_res/emoji_1.png"));
        _open_file_btn = new QPushButton(this);
        _open_file_btn->setFixedSize(26,26);_open_file_btn->setIconSize(QSize(16,16));
        _open_file_btn->setIcon(QIcon(":/res/chat_res/addfile_1.png"));
        _send_btn = new QPushButton("发送",this);
        _send_btn->setFixedSize(80,30);

        QHBoxLayout* lay_44 = new QHBoxLayout(wid_22);
        lay_44->addWidget(_emoji_btn,0, Qt::AlignHCenter);
        lay_44->addWidget(_open_file_btn,0, Qt::AlignHCenter);
        lay_44->addStretch();
        lay_44->addWidget(_send_btn,0, Qt::AlignHCenter);

        lay_44->setContentsMargins(0,0,0,0);
        lay_44->setSpacing(10);

        ///wid-10  listView
        QHBoxLayout* lay_20 = new QHBoxLayout();
        lay_20->setContentsMargins(0,0,0,0);
        wid_10->setLayout(lay_20);

        lay_20->addWidget(_chat_list_view);


        ///右边 stack_r 第二个界面： wddd_2:上面是一个标签，下边是一个listview
        QVBoxLayout* laaa_2 = new QVBoxLayout(wddd_2);
        QLabel* labbb_2 = new QLabel("新的好友申请", this);
        labbb_2->setStyleSheet("color: #2c3e50;font-size: 20px; ");


        laaa_2->setContentsMargins(6,10,1,5);
        laaa_2->setSpacing(6);
        _friend_request_view = new QListView(this);
        laaa_2->addWidget(labbb_2, 0, Qt::AlignCenter);
        laaa_2->addWidget(_friend_request_view);

    }

    _send_file_dialog = new sendFileDialog(this);
    _send_file_dialog->hide();

}

void chatDialog_code::readQss(){
    QFile qss(":/style/chatDialogSheet.qss");
    if(qss.open(QFile::ReadOnly)){
        qDebug("open chatDialogQss success!");
        QString style = QLatin1String(qss.readAll());
        this->setStyleSheet(style);
        qss.close();
    }else{
        qDebug("open chatDialogQss failed!");
    }
}

void chatDialog_code::updateSideBtn(QPushButton *btn,int index){
    _chat_btn->setEnabled(true);
    _chat_btn->setIcon(QIcon(":/res/chat_res/chat_2.png"));
    _friend_btn->setEnabled(true);
    _friend_btn->setIcon(QIcon(":/res/chat_res/friend_2.png"));
    _collect_btn->setEnabled(true);
    _collect_btn->setIcon(QIcon(":/res/chat_res/collect_2.png"));
    _file_btn->setEnabled(true);
    _file_btn->setIcon(QIcon(":/res/chat_res/file_2.png"));

    btn->setEnabled(false);

    if(index == 1) {_add_btn->setIcon(QIcon(":/res/chat_res/add_1.png")); return ;};
    if(index == 2) {_add_btn->setIcon(QIcon(":/res/chat_res/addfriendBtn_1.png")); return ;};
}

////ChatTextEdit 发送信号的槽函数
void chatDialog_code::sendMessageSlots(QString message){
    qDebug()<<"the send message signals send success,the message is : "<<message;

    MessageItem item;
    item.uid           = UserMgr::getInstance()->getUid();
    item.marking       = QUuid::createUuid().toString();
    item.senderId      = "self";
    item.text          = message;
    item.avatar        = UserMgr::getInstance()->getIcon();
    item.timestamp     = QDateTime::currentDateTime();
    item.status        = MessageStatus::Sending;
    item.type          = MessageType::NormalMessage;

    qDebug()<<"now_uid is : "<<_now_uid;
    _user_list_model->findItemToInsertMessage(_now_uid , item);       /// 更新用户列表中的 chatdata
    _chat_list_model->appendMessage(item);                            /// 添加当前项至聊天信息界面

    ///--- 滚动到底部 ---
    QTimer::singleShot(0, this, [this]() {
        QModelIndex lastIndex = _chat_list_model->index(_chat_list_model->rowCount() - 1, 0);
        _chat_list_view->scrollTo(lastIndex, QListView::PositionAtBottom);

    });


    QJsonObject obj;
    obj["selfuid"] = UserMgr::getInstance()->getUid();
    obj["touid"] = _now_uid;
    obj["marking"] = item.marking;
    obj["message"] = message;
    obj["type"] = 1;

    QJsonDocument docu(obj);
    QByteArray byte = docu.toJson(QJsonDocument::Compact);
    TcpMgr::getInstance()->sig_send_data(ReqId::ID_TEXT_CHAT_MSG_REQ,byte);

}

void chatDialog_code::getFriendListMessage(int uid)          /// 通过 uid 获取用户好友列表
{
    qDebug()<<"-----------------------------------load friend list init ...";
    QJsonObject obj;
    obj["selfuid"] = uid;
    QJsonDocument docu(obj);
    QByteArray jsonData = docu.toJson(QJsonDocument::Compact);
    emit TcpMgr::getInstance()->sig_send_data(ReqId::ID_GET_FRIEND_LIST_FROM_UID_REQ,jsonData);
}

void chatDialog_code::getFriendRequestListMessage(int uid)
{
    qDebug()<<"-----------------------------------load friend apply list init ...";
    QJsonObject obj;
    obj["selfuid"] = uid;
    QJsonDocument docu(obj);
    QByteArray jsonData = docu.toJson(QJsonDocument::Compact);
    emit TcpMgr::getInstance()->sig_send_data(ReqId::ID_GET_FRIEND_REQUESTLIST_FROM_UID_REQ,jsonData);
}

void chatDialog_code::chatBtnClicked()
{
    updateSideBtn(_chat_btn,1);
    _main_stack->setCurrentIndex(0);
    _stack_l->setCurrentIndex(0);
    _stack_r->setCurrentIndex(0);
    ///StackedWidgetAnimator::slideToIndex(_stack_r, 0, 300,SlideDirection::Left);

    if(_search_enum == SEARCHFRIENDFROMSERVE)_friend_list_model->restoreData();  /// 复原frienglist数据
    _search_enum = SEARCHCHAT;
}

void chatDialog_code::friendBtnClicked()
{
    updateSideBtn(_friend_btn,2);
    _main_stack->setCurrentIndex(0);
    _stack_l->setCurrentIndex(1);
    _stack_r->setCurrentIndex(1);
    ///StackedWidgetAnimator::slideToIndex(_stack_r, 1, 300,SlideDirection::Right);
    _search_enum = SEARCHFRIENDFROMLIST;

    if(!_is_init_friend_apply){
        _is_init_friend_apply = true;
        /// 加载好友申请列表
        getFriendRequestListMessage(UserMgr::getInstance()->getUid());
    }
}

void chatDialog_code::addBtnSlots(){    /// 搜索框右侧的添加按钮
    if(_stack_l->currentIndex() == 0){    /// 聊天界面，添加群聊功能

        qDebug()<<"添加群聊功能...新开一个窗口添加群聊...";
        return ;
    }
    if(_stack_l->currentIndex() == 1){    /// 好友列表界面，添加好友功能

        qDebug()<<"添加好友功能...";
        if(_search_enum == SEARCHFRIENDFROMLIST){       /// 点击之后变为添加好友功能
            _line_edit->clear();
            _search_enum = SEARCHFRIENDFROMSERVE;
            _add_btn->setIcon(QIcon(":/res/chat_res/close_1.png"));

            QList<FriendItem> templist;
            FriendItem ite;
            ite.name = "搜索：name/uid";
            ite.avatar = ":/res/chat_res/searchList.png";
            ite.isTopItem = true;
            templist.append(ite);

            _friend_list_model->replaceWithTemp(templist);     ///高速替换数据

            _line_edit->setFocus();

            return ;
        }
        if(_search_enum == SEARCHFRIENDFROMSERVE){      /// 点击之后变为查找好友功能
            _line_edit->clear();
            _search_enum = SEARCHFRIENDFROMLIST;
            _add_btn->setIcon(QIcon(":/res/chat_res/addfriendBtn_1.png"));

            _friend_list_model->restoreData();         ///复原数据

            return;
        }
    }
}

void chatDialog_code::initMainChatWid()
{
    _chat_btn->setEnabled(false);
    _send_btn->setEnabled(false);
    _text_edit->setEnabled(false);

    /// 清除四个模型中的数据
    _friend_list_model->initModel();
    _chat_list_model->initModel();
    _user_list_model->initModel();
    _friend_request_model->initModel();

    /// 清除变量值
    _request_send_panding = false;
    _is_init_friend_apply = false;
    _now_uid = 0;
    _search_enum= SEARCHCHAT;
    is_recv_file = false;
    _name_label->setText("");

    chatBtnClicked();

    qDebug()<<"uid is : "<<UserMgr::getInstance()->getUid();
    /// 加载好友列表及其聊天记录
    getFriendListMessage(UserMgr::getInstance()->getUid());
}


void chatDialog_code::changeToFriendRequestViewSlot()          /// 第一项双击
{
    _main_stack->setCurrentIndex(0);
    if(_search_enum == SEARCHFRIENDFROMLIST){  /// 申请列表，转到好友申请界面
        _stack_r->setCurrentIndex(1);
        ///StackedWidgetAnimator::slideToIndex(_stack_r, 1, 300,SlideDirection::Right);
        return ;
    }
    if(_search_enum == SEARCHFRIENDFROMSERVE){ /// 查找新朋友
        ///_stack_r->setCurrentIndex(2);
        ///StackedWidgetAnimator::slideToIndex(_stack_r, 2, 300,SlideDirection::Down);


        if(_request_send_panding) return ;
        // 可以弹出一个模态对话框阻塞其他响应

        auto req = _line_edit->text();
        if(req =="") return ;
        _request_send_panding = true;          /// 请求空闲时,发送查找用户给服务器

        QJsonObject obj;
        obj["uid"] = req;

        QJsonDocument docu(obj);
        QByteArray jsonData = docu.toJson(QJsonDocument::Compact);
        emit TcpMgr::getInstance()->sig_send_data(ReqId::ID_SEARCH_USER_REQ,jsonData);
        qDebug()<<"get a search request to serve: "<<req;

        return ;
    }

}

void chatDialog_code::changeToFriendInfoWid(std::shared_ptr<SearchInfo>info){       /// 其他项双击时
    _friend_info_wid->setNewFriendWId(info, true);

    _stack_r->setCurrentIndex(2);
    ///StackedWidgetAnimator::slideToIndex(_stack_r, 2, 300,SlideDirection::Down);
}

void chatDialog_code::lineEidtChangeFinish()     /// 回车键按下时
{

}

void chatDialog_code::lineEidtChange(const QString &text)           /// 用户编辑时
{
    if(_search_enum == SEARCHFRIENDFROMSERVE){
        if(text.length() == 0) _friend_list_model->chatSearchDataQString("name/uid");
        else _friend_list_model->chatSearchDataQString(text);
        return;
    }
}

void chatDialog_code::_friend_info_wid_addFriendSlo()   /// 朋友信息界面的添加好友按钮按下的槽函数
{
    qDebug()<<"add friend btn slot...";

    QString myname = UserMgr::getInstance()->getName();
    QString tipname = _friend_info_wid->getUserName();
    int tipuid = _friend_info_wid->getUserUid();
    ///qDebug()<<"tipuid is "<<tipuid;
    _apply_friend_wid->updataWid(myname,tipname,tipuid);

    _stack_r->setCurrentIndex(3);
    ///StackedWidgetAnimator::slideToIndex(_stack_r, 3, 300,SlideDirection::Up);
}

void chatDialog_code::applyFriendWidNoBtnSlots()        /// 申请朋友界面取消按钮的槽函数
{
    qDebug()<<"apply friend no btn slot...";
    _stack_r->setCurrentIndex(2);
    ///StackedWidgetAnimator::slideToIndex(_stack_r, 2, 300,SlideDirection::Down);
}

void chatDialog_code::slot_send_btn_clicked()           /// 发送消息槽函数
{
    if(_text_edit->toPlainText() == "") return ;
    QString mess = _text_edit->toPlainText();
    _text_edit->clear();
    sendMessageSlots(mess);
}

void chatDialog_code::slot_apply_friend_send_result(int err)       /// 发送请求添加好友成功
{
    if(err != 0 ){
        new TipWidget("添加好友失败，错误码：123",
                      QColor(200, 50, 50, 220),
                       TipWidget::FAILED,2000,
                      this);
        _stack_r->setCurrentIndex(2);
        return ;
    }
    /// 弹出一个提示框，文本为： 添加好友请求发送成功！
    new TipWidget("添加好友请求发送成功！",
                  QColor(50, 160, 50, 220),
                   TipWidget::SUCCESS,2000,
                  this);

    _stack_r->setCurrentIndex(2);

}

void chatDialog_code::slot_user_search_finish(std::shared_ptr<SearchInfo>info)
{
    _request_send_panding = false;
    qDebug()<<"inter the slot_user_search_finish...";

    /// 分两种情况，该用户已经是好友了，或者不是

    int useruid = (*info)._uid;
    qDebug()<<"-----useruid is: "<<useruid;
    bool isexist = _friend_list_model->checkUidIsExist(useruid);     /// 已经是好友了
    qDebug()<<"the user exist is: "<<isexist<<"-------------------";

    _friend_info_wid->setNewFriendWId(info, isexist);

    _stack_r->setCurrentIndex(2);
}

void chatDialog_code::slot_user_search_failed(int e)
{
    Q_UNUSED(e);
    qDebug()<<"inter the slot_user_search_failed...";
    _request_send_panding = false;
    new TipWidget("没有找到该位用户哦 ！",
                  QColor(200, 150, 50, 220),
                  TipWidget::WARNING,1500,
                  this);
}

void chatDialog_code::tcp_slot_new_friend_apply_comming(std::shared_ptr<FriendRequest> item)
{
    qDebug()<<"There has a new friend apply coming...";
    _friend_request_model->addRequest(*item,0);
}

/// 同意对方好友申请 回包（对方发来的好友请求，我同意）
void chatDialog_code::slot_ID_AUTH_FRIEND_RSP_callback(std::shared_ptr<FriendItem>item){  /// 同意对方好友申请 回包

    _friend_list_model->addItem(*item,1);                                 /// 1.将好友添加到好友列表中
    _friend_request_model->setItemAcceptfromUid((*item).uid);             /// 2.将好友申请列表中的该条数据 accept 设置为 true

    /// 3. 给该好友发送一句信息“我已添加了你的好友，现在我们可以开始聊天了！”
    _user_list_model->findTheItemFromUid((*item).uid, item);      /// 2. 加添此条记录到聊天列表，展示聊天信息
    MessageItem i;
    i.uid           = UserMgr::getInstance()->getUid();
    i.marking       = QUuid::createUuid().toString();
    i.senderId      = "self";
    i.text          = "我已添加了你的好友，现在我们可以开始聊天了！";
    i.avatar        = UserMgr::getInstance()->getIcon();
    i.timestamp     = QDateTime::currentDateTime();
    i.status        = MessageStatus::Sending;
    i.type          = MessageType::NormalMessage;
    _user_list_model->findItemToInsertMessage(item->uid , i);

    QJsonObject obj;
    obj["selfuid"] = UserMgr::getInstance()->getUid();
    obj["touid"] = item->uid;
    obj["marking"] = i.marking;
    obj["message"] = i.text;
    obj["type"] = 1;

    QJsonDocument docu(obj);
    QByteArray byte = docu.toJson(QJsonDocument::Compact);
    TcpMgr::getInstance()->sig_send_data(ReqId::ID_TEXT_CHAT_MSG_REQ,byte);

}

/// 我给对方发送的好友申请，对方同意了的 GRPC 回调
void chatDialog_code::slot_ID_NOTIFY_AUTH_FRIEND_REQ_callback(std::shared_ptr<FriendItem> item){

    _friend_list_model->addItem(*item,1);                         /// 1. 将该好友添加进好友列表中




}

void chatDialog_code::slot_updateMessage(int useruid, QString marking)         ///发送成功后的槽函数
{
    _user_list_model->SetfindItemToInsertMessage(useruid , marking);
    if(useruid == _now_uid){
        _chat_list_model->updateMessageStatus(marking,MessageStatus::Sent);
    }
}

/// 有好友发信息给我的 GRPC 槽函数
void chatDialog_code::slot_notify_updateMessage(int selfuid, int touid, QString marking, QString message,int type){
    if(touid != UserMgr::getInstance()->getUid()) return;
    /// 1. 查找聊天列表中有无该好友
    int index = 0;
    bool isfind = _user_list_model->findUid(selfuid,index);
    if(isfind){
        MessageItem item = _user_list_model->addMessageItem(index,marking,message,type);
        if(selfuid == _now_uid) _chat_list_model->appendMessage(item);
        return;
    }
    /// 聊天列表中没有找到该好友，则插入该好友聊天列表
    FriendItem* items = _friend_list_model->getItemFromUid(selfuid);
    chatUserListItem item;
    item.uid = items->uid ;
    item.name = items->name;
    item.avatar = items->avatar;
    _user_list_model->addItem(item,0);

    MessageItem it = _user_list_model->addMessageItem(0,marking,message,type);
    if(selfuid == _now_uid) _chat_list_model->appendMessage(it);
    return;

}

void chatDialog_code::slot_userListDoubleClicked(QString name, QList<MessageItem> data,int nowuid)
{
    if(!_send_btn->isEnabled())_send_btn->setEnabled(true);
    if(!_text_edit->isEnabled())_text_edit->setEnabled(true);
    _chat_list_model->setItems(data);
    _name_label->setText(name);
    _now_uid = nowuid;

}

void chatDialog_code::slot_getFriendListFromUid(QList<FriendItem> list)
{
    ///qDebug()<<"-----------------------------------------load friend list callback...";
    _friend_list_model->setItems(list);
    ///qDebug()<<"----------------设置好友列表成功---------------------------";

    /// 加载聊天列表
    QList<chatUserListItem> chatlist;
    for(const auto& item : list){
        chatUserListItem it{item.name,item.avatar,"","",item.uid,{}};
        chatlist.append(it);
    }
    _user_list_model->setItems(chatlist);
    ///qDebug()<<"----------------设置聊天列表成功---------------------------";

    /// 加载聊天列表的聊天记录
    ///qDebug()<<"-----------------------------------------load chat message init...";
    _user_list_model->getChatMessageList(0,_user_list_model->rowCount());
}

void chatDialog_code::slot_getFriendRequestListFromUid(QList<FriendRequest> list)
{
    _friend_request_model->setRequests(list);
}

void chatDialog_code::slot_message_data_to_update(int & useruid, int & index, QList<MessageItem> list)
{
    ///qDebug()<<"-----------------------------------------load chat message list callback...";

    if(_user_list_model->getUidFromIndex(index) != useruid) {
        qDebug()<<"获取聊天记录时回包中对方uid 和 客户端中不一致，异常退出...";
        return ;
    }

    for( auto& it: list){
        it.avatar = _user_list_model->getIconFromIndex(index);
        it.senderId = it.uid == useruid ? _user_list_model->getNameFromIndex(index) : "self";
    }
    _user_list_model->setMessageFromIndex(index,list);

    /// 加载下一个好友的聊天记录
    _user_list_model->getChatMessageList(index+1,_user_list_model->rowCount());
}

void chatDialog_code::slot_message_data_to_prepend(int &uid, QString &messMarking, QString &loadMarking, QList<MessageItem> list)
{
    _chat_list_view->setCanEmit(true);
    int row = list.count();

    if(list.empty()) {
        qDebug()<<"list为空，没有更早的消息了...";
        _chat_list_model->setLoadingToTimeStyle(loadMarking,row);
        return ;
    }
    int index = _user_list_model->getIndexFromUid(uid);
    if(index == -1){
        qDebug()<<"服务器回包中的uid 与 聊天列表中的 uid ,不一致，异常退出...";
        return ;
    }
    for( auto& it: list){
        it.avatar = _user_list_model->getIconFromIndex(index);
        it.senderId = it.uid == uid ? _user_list_model->getNameFromIndex(index) : "self";
    }
    /// 将记录添加到列表中
    _user_list_model->appendMessageFromIndex(index,list,messMarking);

    if(_now_uid == uid){       /// 还是在与这个对象聊天的话，加载
        _chat_list_model->insertItems(list,0,_chat_list_view);

        _chat_list_model->setLoadingToTimeStyle(loadMarking,row);

    }

}

void chatDialog_code::_msg_btn_clicked_slot(int uid)         /// 好友信息界面中发送消息的按钮按下的槽函数
{
    chatBtnClicked();
    /// 加载其聊天信息
    QString name;
    QList<MessageItem> messages;
    bool is_ok = _user_list_model->getNameAndChatMessageFromUid(uid,name,messages);
    if(is_ok){
        chatBtnClicked();
        slot_userListDoubleClicked(name,messages,uid);
    }
}

void chatDialog_code::loadMoreChatMessage()
{
    if(_now_uid == 0) {
        _chat_list_view->setCanEmit(true);
        return ;
    }

    bool i = _chat_list_model->getFrontItem().type == MessageType::TimeItem;
    if(i) {
        _chat_list_view->setCanEmit(true);
        return ;
    }

    int uid = _now_uid;
    QString marking = _chat_list_model->getFrontMarking();
    int selfuid = UserMgr::getInstance()->getUid();
    qDebug()<<"load more chat messages , selfuid is : "<<selfuid<<"    uid is : "<<uid<<"  marking is : "<<marking;

    /// 设置代理绘制动画标识
    MessageItem item;
    item.marking = QUuid::createUuid().toString();
    item.type = MessageType::LoadMessage;
    _chat_list_model->insertItem(item,0);

    /// tcp 发送请求
    QJsonObject obj;
    obj["selfuid"] = selfuid;
    obj["touid"] = uid;
    obj["messageMarking"] = marking;
    obj["loadingMarking"] = item.marking;

    QJsonDocument docu(obj);
    QByteArray byte = docu.toJson(QJsonDocument::Compact);

    /// 使用 tcpMgr 给服务器发送请求
    TcpMgr::getInstance()->sig_send_data(ReqId::ID_LOADING_MESSAGE_FROM_UID_REQ,byte);

}

void chatDialog_code::fileDropped_slot(QString filePath)      ////  文件拖进来的槽函数： 文件绝对路径
{
    if(_send_file_dialog == nullptr) _send_file_dialog = new sendFileDialog(this);

    /// 更新用户信息
    QString icon,name;
    bool is = _user_list_model->getIconAndNameFromUid(_now_uid,icon,name);
    if(!is) {
        qDebug()<<"is false, failed to set...";
        return ;
    }
    if(icon == "") icon = ":/res/chat_res/self_1.png";

    _send_file_dialog->SendsetUserInfo(icon,name);

    /// 更新文件信息
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists()) {
        qDebug() << "文件不存在！";
        return;
    }

    /// 获取后缀（统一为小写）
    QString suffix = fileInfo.suffix().toLower();

    /// 1. 禁止发送快捷方式
    if (suffix == "lnk") {
        new TipWidget("暂不支持发送 .lnk 快捷方式！",
                      QColor(255, 50, 100, 200),
                      TipWidget::WARNING, 2000,
                      this);
        return;
    }

    /// 2. 禁止发送文件夹
    if (fileInfo.isDir() || suffix.isEmpty()) {
        new TipWidget("暂不支持发送文件夹",
                      QColor(255, 220, 100, 240),
                      TipWidget::WARNING, 2000,
                      this);
        return;
    }

    /// 3. 禁止发送可执行文件
    if (suffix == "exe" || suffix == "msi" || suffix == "bat" ||
        suffix == "cmd" || suffix == "com" || suffix == "scr" ||
        suffix == "pif" || suffix == "cpl" || suffix == "js" ||
        suffix == "vbs" || suffix == "wsf" || suffix == "ps1") {
        new TipWidget("为安全起见，禁止发送可执行类文件（.exe 等）",
                      QColor(255, 100, 50, 200),
                      TipWidget::WARNING, 2500,
                      this);
        return;
    }

    /// 4. 可选：禁止发送系统驱动或内核文件（一般用户不会发，但保险起见）
    if (suffix == "sys" || suffix == "dll" || suffix == "drv" || suffix == "ocx") {
        new TipWidget("禁止发送系统关键文件（.dll, .sys 等）",
                      QColor(200, 50, 50, 200),
                      TipWidget::WARNING, 2000,
                      this);
        return;
    }
    QString fileName = fileInfo.fileName();         /// 如 "test.cpp"
    qint64 size = fileInfo.size();                  /// 如 1024
    /// 如果大小大于四个G， 则无法传输
    if (size > 4294967296LL) {      /// 判断是否超过4GB
        new TipWidget("暂不支持发送超过4GB的文件",
                      QColor(255, 100, 100, 240),
                      TipWidget::WARNING, 2000,
                      this);
        return;
    }

    QString sizeStr = formatFileSize(size);         /// 如 233K 、3.5M 、 1.2G
    _send_file_dialog->setFileInfo(fileName,suffix,size,sizeStr,filePath);

    /// 更新进度条, 隐藏
    _send_file_dialog->setProgressBar(false);

    _send_file_dialog->exec();
}

void chatDialog_code::slot_getFileServerInfo(QString host, QString port,int is_send)
{
    tcpFilemgr::getInstance()->slot_connect_server(host, port,is_send);
}

void chatDialog_code::slot_fileserver_socketConnectChange(bool b, int is_send)
{
    if(b){
        if(is_send){
            _send_file_dialog->sendFileSlot();
        }else{
            _send_file_dialog->recvFileSlot();
        }

    }else{
        _send_file_dialog->hide();
        _send_file_dialog->setUIEnable(true);
        _send_file_dialog->setProgressBar(false);
        new TipWidget("无法连接到文件服务器 !",
                      QColor(255, 100, 100, 240),
                      TipWidget::FAILED, 2000,
                      this);
    }
}

void chatDialog_code::slot_send_file_callback(quint16 msg_id, const QJsonObject &jsonObj){

    auto error =jsonObj.value("error").toInt();
    if(error != ErrorCodes::SUCCESS){
        qDebug()<<"send file ,the callback error is not ErrorCodes::SUCCESS";
        return ;
    }

    if(msg_id == ReqId::ID_UPLOAD_FILE_RSP) {

        qint64 trans_size = jsonObj.value("trans_size").toString().toLongLong();
        qint64 total_size = jsonObj.value("total_size").toString().toLongLong();

        auto name = jsonObj.value("name").toString();
        auto last = jsonObj.value("last").toInt();

        if (trans_size > 0 && total_size > 0) {
            _send_file_dialog->updateProgress(trans_size, total_size);
        }

        if(last){      /// 文件已经上传完
            qDebug()<<"文件已经上传完！";

            _send_file_dialog->hide();
            _send_file_dialog->setUIEnable(true);

            /// 1. 给对方发送一个文件消息
            QString filesize = formatFileSize(total_size);          /// 文件大小
            QString suffix = QFileInfo(name).suffix();              /// 文件后缀
            QString marking = jsonObj.value("marking").toString();

            addFileItems(name,filesize,suffix,marking, true);       /// 文件名、文件大小，文件后缀，文件标识，是否自己发的


        }
        return ;
    }

    if(msg_id == iD_GET_FILE_FROM_MARKING_RSP){

        int code = jsonObj.value("code").toInt();
        if(code != 0){
            new TipWidget("无法加载到服务器的文件 !",
                                    QColor(255, 100, 100, 240),
                                    TipWidget::FAILED, 2000,this);
            _send_file_dialog->hide();
            _send_file_dialog->setUIEnable(true);
            return ;
        }
        /// 提取字段
        QString req = jsonObj.value("req").toString();
        QString total_req = jsonObj.value("total_req").toString();
        QString base64_data = jsonObj.value("data").toString();
        bool is_last = jsonObj.value("last").toBool();
        QString marking = jsonObj.value("marking").toString();
        QString filename = jsonObj.value("filename").toString();

        ///qDebug() << "接收到第 " << req << " / " << total_req << " 包" ;
        ///qDebug() << "总包数： "<<total_req;
        qint64 trans_size = jsonObj.value("req").toString().toLongLong();
        qint64 total_size = jsonObj.value("total_req").toString().toLongLong();



        /// 如果是第一包，初始化文件接收
        if (req == "1") {
            this->is_recv_file = true;
            this->marking = marking;

            QString app_path = QCoreApplication::applicationDirPath();
            QString fileName = "config.ini";
            QString config_path = QDir::toNativeSeparators(app_path + QDir::separator() + fileName);
            QSettings settings(config_path, QSettings::IniFormat);

            QString RESOURCE_PATH = settings.value("static/path").toString();   /// 资源文件夹路径
            qDebug() << "从 config.ini 读取的 RESOURCE_PATH 路径为：" << RESOURCE_PATH;

            /// 确保资源目录存在
            QDir dir;
            if (!dir.exists(RESOURCE_PATH)) {
                dir.mkpath(RESOURCE_PATH);
            }

            /// 拼接目标文件路径，使用原始文件名加 .recv
            QString recv_filename = filename + ".recv";
            this->recv_file_path = QDir::toNativeSeparators(RESOURCE_PATH + QDir::separator() + recv_filename);
            qDebug()<<"本次文件下载的保存位置是: "<<recv_file_path;

            // 创建或清空目标文件
            QFile file(this->recv_file_path);
            if (file.open(QIODevice::WriteOnly)) {
                file.close();
            }
        }

        _send_file_dialog->updateProgress(trans_size, total_size);

        // base64 解码成 QByteArray
        QByteArray decoded = QByteArray::fromBase64(base64_data.toUtf8());

        // 将数据追加写入文件
        QFile file(this->recv_file_path);
        if (file.open(QIODevice::Append)) {
            file.write(decoded);
            file.close();
        }

        // 如果是最后一包
        if (is_last) {

            qDebug() << "文件接收完毕：" << this->recv_file_path;

            /// 去掉 .recv 后缀，恢复原始文件名
            QString final_file_path = this->recv_file_path;
            final_file_path.chop(5);                           /// 删除 ".recv"

            /// 如果存在同名文件就先删除
            QFile::remove(final_file_path);

            if (QFile::rename(this->recv_file_path, final_file_path)) {
                qDebug() << "重命名成功，最终文件：" << final_file_path;
            } else {
                qDebug() << "重命名失败";
            }
            is_recv_file = false;
            this->marking.clear();

            _send_file_dialog->hide();
            _send_file_dialog->setUIEnable(true);

            new TipWidget("文件接收成功，点击“打开”查看文件夹！",
                          QColor(50, 160, 50, 220),
                          TipWidget::SUCCESS,2000,
                          this);

            return;
        }

        return ;

    }

    qDebug()<<"回包中的 id 不匹配，异常退出...";
    return;



}

void chatDialog_code::addFileItems(QString &name, QString &size, QString &suffix,QString& marking, bool isself)
{
    MessageItem item;
    item.uid = isself? UserMgr::getInstance()->getUid() : _now_uid;

        QString icon,username;
        _user_list_model->getIconAndNameFromUid(_now_uid,icon,username);

    item.avatar = isself ? UserMgr::getInstance()->getIcon() : icon;
    item.marking = marking;
    item.senderId = isself ? "self" : username;
    item.timestamp = QDateTime::currentDateTime();
    item.type = MessageType::File;
    item.status = MessageStatus::Sent;

    item.text = name + " " + size + " " + getFileIconFromSuffix(suffix) ;
    _user_list_model->getItemFromUid(_now_uid).chatdata.append(item);
    _chat_list_model->appendMessage(item);      /// 添加进聊天界面


    ///--- 滚动到底部 ---
    QTimer::singleShot(0, this, [this]() {
        QModelIndex lastIndex = _chat_list_model->index(_chat_list_model->rowCount() - 1, 0);
        _chat_list_view->scrollTo(lastIndex, QListView::PositionAtBottom);

    });


    QJsonObject obj;
    obj["selfuid"] = UserMgr::getInstance()->getUid();
    obj["touid"] = _now_uid;
    obj["marking"] = item.marking;
    obj["message"] = item.text;
    obj["type"] = 3;

    QJsonDocument docu(obj);
    QByteArray byte = docu.toJson(QJsonDocument::Compact);
    TcpMgr::getInstance()->sig_send_data(ReqId::ID_TEXT_CHAT_MSG_REQ,byte);

}

void chatDialog_code::slot_downloadClicked(const QModelIndex &index)
{
    qDebug()<<"下载文件按钮触发...";
    /// 获取文本内容
    QString textMarking = index.data(chatMessageListMOdel::MarkingRole).toString();

    /// 获取文本内容
    QString textContent = index.data(chatMessageListMOdel::TextRole).toString();
    ///qDebug() << "文件内容为 ：" << textContent;

    QStringList parts = textContent.split(" ", Qt::SkipEmptyParts);
    /// 使用 .value() 提取每一部分
    QString filename_    = parts.value(0);  // 第一个字段
    QString filesize     = parts.value(1);  // 第二个字段
    QString fileIcon     = parts.value(2);  // 第三个字段

    /// 更新用户信息
    QString isself = index.data(chatMessageListMOdel::SenderIdRole).toString();
    if(isself == "self"){
        _send_file_dialog->RecvsetUserInfo(UserMgr::getInstance()->getIcon(),
                                           UserMgr::getInstance()->getName());
    }
    else{
        QString icon,name;
        bool is = _user_list_model->getIconAndNameFromUid(_now_uid,icon,name);
        if(!is) {
            qDebug()<<"is false, failed to set...";
            return ;
        }
        if(icon == "") icon = ":/res/chat_res/self_1.png";
        _send_file_dialog->RecvsetUserInfo(icon,name);

    }
    /// 更新文件信息
    _send_file_dialog->setRecvFileInfo(filename_,filesize,fileIcon,textMarking);

    /// 更新进度条, 隐藏
    _send_file_dialog->setProgressBar(false);
    _send_file_dialog->setUIEnable(true);
    _send_file_dialog->exec();

}

void chatDialog_code::slot_openClicked(const QModelIndex &index)
{
    qDebug()<<"打开文件按钮触发...";
    /// 获取文本内容
    QString textContent = index.data(chatMessageListMOdel::TextRole).toString();

    QStringList parts = textContent.split(" ", Qt::SkipEmptyParts);
    QString filename_    = parts.value(0);  // 第一个字段
    qDebug() << "文件名为 ：" << filename_;

    /// 查看 资源文件夹下有无该文件
    QString app_path = QCoreApplication::applicationDirPath();
    QString fileName = "config.ini";
    QString config_path = QDir::toNativeSeparators(app_path + QDir::separator() + fileName);
    QSettings settings(config_path, QSettings::IniFormat);

    QString RESOURCE_PATH = settings.value("static/path").toString();   /// 资源文件夹路径
    qDebug() << "资源文件夹路径为 ：" << RESOURCE_PATH;
    /// 拼接完整文件路径
    QString fullFilePath = QDir::toNativeSeparators(RESOURCE_PATH + QDir::separator() + filename_);

    /// 检查文件是否存在
    QFileInfo checkFile(fullFilePath);
    if (checkFile.exists() && checkFile.isFile()) {
        qDebug() << "文件存在：" << fullFilePath;

    /// 打开资源文件夹并定位本文件
#ifdef Q_OS_WIN
        QString nativePath = QDir::toNativeSeparators(fullFilePath);
        QStringList args;
        args << "/select," + nativePath;
        bool ok = QProcess::startDetached("explorer.exe", args);
        qDebug() << "启动 explorer 状态：" << ok << " 参数：" << args;
#else
        QDesktopServices::openUrl(QUrl::fromLocalFile(checkFile.absolutePath()));
#endif

    } else {
        qDebug() << "文件不存在：" << fullFilePath;
        new TipWidget("未找到本地文件，请先下载!",
                      QColor(255, 150, 100, 240),
                      TipWidget::WARNING, 2000,this);
        return;
    }
}

void chatDialog_code::NotifyOfflineSlot(int uid)
{
    if(uid != UserMgr::getInstance()->getUid()){
        qDebug()<<"异地登陆离线信号，但是 uid 并不是当前的 uid 呢...";
        return;
    }

    CustomDialog *dlg = new CustomDialog("账号异地登陆，点击切换账号", CustomDialog::StatusEnum::WARNING, this);
    dlg->exec();  // 模态显示
    emit sigChatChangeToLogWid();
}

void chatDialog_code::heartOuttimeOffSocketSlot()
{
    CustomDialog *dlg = new CustomDialog("心跳超时或网络异常，点击重新登录", CustomDialog::StatusEnum::FAILED, this);
    dlg->exec();  // 模态显示
    emit sigChatChangeToLogWid();
}



























