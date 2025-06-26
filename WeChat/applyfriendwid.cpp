#include "applyfriendwid.h"
#include "ui_applyfriendwid.h"
#include "usermgr.h"
#include <QJsonDocument>
#include <QJsonObject>
#include "tcpmgr.h"

applyFriendWId::applyFriendWId(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::applyFriendWId),_user_uid(0)
{
    ui->setupUi(this);

    connect(ui->no,&QPushButton::clicked,[this](){emit this->noBtnClickedSignals();});
    connect(ui->ok,&QPushButton::clicked,this,&applyFriendWId::slotOkBtnClicked);
}

applyFriendWId::~applyFriendWId()
{
    delete ui;
}

void applyFriendWId::updataWid(QString myname, QString tipname,int uid)
{
    ui->requestline->setText("我是：" + myname);
    ui->noteline->setText(tipname);
    _user_uid = uid;


}

void applyFriendWId::slotOkBtnClicked()
{
    QString self_name = UserMgr::getInstance()->getName();
    int self_uid = UserMgr::getInstance()->getUid();

    QJsonObject jsonobj;
    jsonobj["selfname"] = self_name;
    jsonobj["selfuid"] = self_uid;
    jsonobj["touid"] = _user_uid;
    jsonobj["message"] = ui->requestline->text();
    jsonobj["nick"] = ui->noteline->text();
    qDebug()<<"user uid is: "<<_user_uid;

    QJsonDocument docu(jsonobj);
    QByteArray data = docu.toJson(QJsonDocument::Compact);

    /// 发送好友请求 给 chatServer 服务器
    emit TcpMgr::getInstance()->sig_send_data(ReqId::ID_ADD_FRIEND_REQ,data);   // 好友申请 请求

    emit emitFriendApplySignals();      // 给主窗口发送一个信号

}
























