#include "friendinfowidget.h"
#include <QLabel>
#include "myhoverbutton.h"
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include <QFrame>
#include <QEvent>
#include <QMouseEvent>
#include <QString>

friendInfoWidget::friendInfoWidget(QWidget *parent)
    : QWidget{parent}
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAutoFillBackground(false);
    this->initUI();

    this->installEventFilter(this);  // 安装事件过滤器

    this->setStyleSheet("QLabel, QPushButton, QLineEdit { background: transparent; }");

    connect(_add_friend_button,&QToolButton::clicked,[this](){
        emit this->_apply_friend_wid_signals();
    });
    connect(_msg_button,&QToolButton::clicked,[this]{
        emit _msg_btn_clicked_signals(getUserUid());
    });
}

void friendInfoWidget::setNewFriendWId(std::shared_ptr<SearchInfo> info, bool isexist)
{
    /// 更新界面
    _wxid_label->setText(QString::number((*info)._uid));
    _date_label->setText((*info)._name);
    _say_say_show->setText((*info)._desc);

    _remark_edit->setEnabled(isexist);
    _msg_button->setEnabled(isexist);
    _audio_button->setEnabled(isexist);
    _video_button->setEnabled(isexist);
    _add_friend_button->setEnabled(!isexist);

}

QString friendInfoWidget::getUserName()
{
    return _date_label->text();
}

int friendInfoWidget::getUserUid()
{
    ///qDebug()<<"user uid is :"<<_wxid_label->text();
    return _wxid_label->text().toInt();
}

void friendInfoWidget::initUI()
{
    // 左边信息部分
    auto wid_1 = new QWidget(this);
    auto vly_1 = new QVBoxLayout;
    wid_1->setLayout(vly_1);

    _date_label = new QLabel("your name", this);
    _wxid_label = new QLabel("your uuid", this);
    vly_1->addWidget(_date_label);
    vly_1->addWidget(_wxid_label);
    vly_1->setContentsMargins(10,1,10,1);
    vly_1->addStretch();

    // 上方头像 + 基本信息
    auto wid_2 = new QWidget(this);
    auto hly_1 = new QHBoxLayout;
    wid_2->setLayout(hly_1);

    _avatar_label = new QLabel(this);
    _avatar_label->setFixedSize(60, 60);
    _avatar_label->setPixmap(QPixmap(":/res/chat_res/self_1.png")
                                 .scaled(60, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    hly_1->addWidget(_avatar_label);
    hly_1->addWidget(wid_1);
    hly_1->addStretch();
    wid_2->setFixedHeight(75);

    // 备注
    auto wid_3 = new QWidget(this);
    auto hly_2 = new QHBoxLayout;
    wid_3->setLayout(hly_2);

    _remark_title_label = new QLabel("备注: ", this);
    _remark_edit = new QLineEdit(this);
    _remark_edit->setPlaceholderText("点击添加备注");
    hly_2->addWidget(_remark_title_label);
    hly_2->addWidget(_remark_edit);
    wid_3->setFixedHeight(60);

    // 签名
    auto wid_6 = new QWidget(this);
    auto hly_6 = new QHBoxLayout;
    wid_6->setLayout(hly_6);

    _say_say_lab = new QLabel("签名: ", this);
    _say_say_show = new QLabel("空空如也~", this);
    hly_6->addWidget(_say_say_lab);
    hly_6->addWidget(_say_say_show);
    hly_6->addStretch();
    wid_6->setFixedHeight(60);

    // 来源
    auto wid_4 = new QWidget(this);
    auto hly_4 = new QHBoxLayout;
    wid_4->setLayout(hly_4);

    _source_title_label = new QLabel("来源: ", this);
    _source_value_label = new QLabel("通过搜索手机号添加", this);
    hly_4->addWidget(_source_title_label);
    hly_4->addWidget(_source_value_label);
    hly_4->addStretch();
    wid_4->setFixedHeight(60);

    // 四个按钮
    auto wid_5 = new QWidget(this);
    auto hl = new QHBoxLayout;
    wid_5->setLayout(hl);

    _msg_button = new myHoverButton(this);
    _audio_button = new myHoverButton(this);
    _video_button = new myHoverButton(this);
    _add_friend_button = new myHoverButton(this);

    _msg_button->setNormalIcon(QIcon(":/res/chat_res/q_send.png"));
    _msg_button->setHoverIcon(QIcon(":/res/chat_res/q_send_1.png"));
    _msg_button->setIconSize(QSize(34,34));
    _msg_button->setFixedSize(QSize(45,45));

    _audio_button->setNormalIcon(QIcon(":/res/chat_res/q_phone.png"));
    _audio_button->setHoverIcon(QIcon(":/res/chat_res/q_phone_1.png"));
    _audio_button->setIconSize(QSize(34,34));
    _audio_button->setFixedSize(QSize(45,45));

    _video_button->setNormalIcon(QIcon(":/res/chat_res/q_video.png"));
    _video_button->setHoverIcon(QIcon(":/res/chat_res/q_video_1.png"));
    _video_button->setIconSize(QSize(34,34));
    _video_button->setFixedSize(QSize(45,45));

    _add_friend_button->setNormalIcon(QIcon(":/res/chat_res/q_friend.png"));
    _add_friend_button->setHoverIcon(QIcon(":/res/chat_res/q_friend_1.png"));
    _add_friend_button->setIconSize(QSize(34,34));
    _add_friend_button->setFixedSize(QSize(45,45));

    hl->addStretch();
    for (auto btn : {_msg_button, _audio_button, _video_button, _add_friend_button}) {
        btn->setCursor(Qt::PointingHandCursor);
        hl->addWidget(btn);
    }
    hl->addStretch();
    hl->setSpacing(30);
    wid_5->setFixedHeight(60);

    // 主区域
    auto mainWid = new QWidget(this);
    mainWid->setFixedSize(QSize(400, 360));

    auto main_lay = new QVBoxLayout;
    mainWid->setLayout(main_lay);

    main_lay->addWidget(wid_2);

    auto line1 = new QFrame;
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);
    main_lay->addWidget(line1);

    main_lay->addWidget(wid_3);
    main_lay->addWidget(wid_6);
    main_lay->addWidget(wid_4);

    auto line2 = new QFrame;
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);
    main_lay->addWidget(line2);

    main_lay->addWidget(wid_5);
    main_lay->setSpacing(2);
    main_lay->addStretch();

    // 外部布局
    auto l = new QHBoxLayout(this);
    l->addStretch();
    l->addWidget(mainWid);
    l->addStretch();
}

bool friendInfoWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QWidget *w = qobject_cast<QWidget*>(childAt(static_cast<QMouseEvent*>(event)->pos()));
        if (!qobject_cast<QLineEdit*>(w)) {
            /// 如果点击的不是 QLineEdit，让当前拥有焦点的控件清除焦点
            QWidget *focus = this->focusWidget();
            if (focus && qobject_cast<QLineEdit*>(focus)) {
                focus->clearFocus();
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}






























