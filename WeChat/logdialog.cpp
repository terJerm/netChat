#include "logdialog.h"
#include "ui_logdialog.h"
#include <QLineEdit>
#include "httpmge.h"
#include "tcpmgr.h"

logDialog::logDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::logDialog),_acount_valid(false),_passwd_valid(false)
{
    ui->setupUi(this);
    ui->password->setEchoMode(QLineEdit::Password);
    setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);

    ui->account->setPlaceholderText("请输入你的邮箱地址");
    ui->password->setPlaceholderText("请输入 8 ~ 15 位的密码");
    ui->log->setEnabled(false);

    connect(ui->registe,&QPushButton::clicked,this,&logDialog::switchRegister);
    connect(ui->forget,&QPushButton::clicked,this,&logDialog::switchForget);
    connect(ui->log,&QPushButton::clicked,this,&logDialog::logSlot);
    connect(ui->account,&QLineEdit::editingFinished,[this](){
        auto email = ui->account->text();
        QRegularExpression regex(R"(((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+))");
        bool match = regex.match(email).hasMatch();
        if(!match){
            showTip("邮箱格式不匹配",false);
            ui->log->setEnabled(false);_acount_valid = false;
            return ;
        }
        ui->tip->clear();
        _acount_valid = true;
        ui->log->setEnabled(_passwd_valid);
    });
    connect(ui->password,&QLineEdit::editingFinished,[this](){
        if(ui->password->text().length() >15||ui->password->text().length()< 8){
            showTip("密码格式不正确",false);
            ui->log->setEnabled(false),_passwd_valid = false;
            return;
        }ui->tip->clear();
        _passwd_valid = true;
        ui->log->setEnabled(_acount_valid);
    });
    connect(httpMge::getInstance().get(),&httpMge::sig_log_mod_finish,this,&logDialog::logHttpFinishSlot);
    connect(this,&logDialog::sig_connect_tcp_server,TcpMgr::getInstance().get(),&TcpMgr::slot_connect_server);
    connect(TcpMgr::getInstance().get(),&TcpMgr::sig_socket_connect_success,this,&logDialog::slot_socket_connect_success);
    connect(TcpMgr::getInstance().get(),&TcpMgr::sig_login_failed,this,&logDialog::sig_login_failed);
    connect(TcpMgr::getInstance().get(),&TcpMgr::sig_switch_chat,[this](){
        emit switchChatDialog();
    });

    initHandles();

}

logDialog::~logDialog()
{
    delete ui;
}

void logDialog::initUI()
{
    _acount_valid = false;
    _passwd_valid = false;
    _uid = 0;
    _token = "";
    showTip("",true);
    ui->account->setText("");
    ui->password->setText("");
    ui->forget->setEnabled(true);
    ui->registe->setEnabled(true);
    ui->log->setEnabled(false);
}

void logDialog::showTip(QString str,bool ex)
{
    ui->tip->setText(str);
    ui->tip->setProperty("state",ex?"normal":"error");
    repolish(ui->tip);
}

void logDialog::initHandles()
{
    _handles.insert(ReqId::ID_LOGIN_USER,[this](QJsonObject obj){
        ///qDebug()<<"处理登录成功后的操作...";
        int err = obj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            showTip("参数错误，登录失败...",false);return;
        }
        showTip("登录成功，等待连接至服务器...",true);
        /**************************************************************/

        //   * @file:      logdialog.cpp
        //   * @brife:     在登录成功后，gateserver网关服务会发送一个tcp长连接的聊天服务器信息
        //                 用来给客户端连接
        //   * @date:      2025/04/12

        /**************************************************************/
        auto email = obj["email"].toString();

        ServerInfo si;
        si.Host = obj["Host"].toString();
        si.Port = obj["Port"].toString();
        si.Uid = obj["Uid"].toInt();
        si.Token = obj["Token"].toString();

        _uid = obj["Uid"].toInt();;
        _token = obj["Token"].toString();

        qDebug()<<"email is : "<<email;
        qDebug()<<" ChatServer Host is : "<<si.Host;
        qDebug()<<" ChatServer Port is : "<<si.Port;
        qDebug()<<" ChatServer Uid is : "<<si.Uid;
        qDebug()<<" ChatServer Token is : "<<si.Token;

        //发送信号通知tcpMgr 发送tcp长连接给聊天服务器
        emit sig_connect_tcp_server(si);
        qDebug()<<"emit a sig_connect_tcp_server signal to system...";

    });
}

void logDialog::switchRegister()
{
    ui->account->clear();
    ui->password->clear();
    _acount_valid = false,_passwd_valid = false,ui->log->setEnabled(false),ui->tip->clear();
    emit switchRgt();
}

void logDialog::switchForget()
{
    ui->account->clear();
    ui->password->clear();
    _acount_valid = false,_passwd_valid = false,ui->log->setEnabled(false),ui->tip->clear();
    emit switchForgetSignal();
}

void logDialog::logSlot()
{
    showTip("正在登陆中，请稍等...",true);
    auto email = ui->account->text();
    auto passwd = ui->password->text();
    QJsonObject obj;
    obj["email"] = email;
    obj["password"] = enhancedXorEncrypt(passwd,key,salt);
    httpMge::getInstance()->PostHttpReq(QUrl(gate_url_prefix+"/user_login"),obj,
                                        ReqId::ID_LOGIN_USER,Modules::LOGMOD);

}

void logDialog::logHttpFinishSlot(ReqId id, QString res, ErrorCodes err)
{
    if( err != ErrorCodes::SUCCESS){
        showTip(tr("网络请求错误"),false);
        return;
    }
    ///反序列化为JsonDOcument
    QJsonDocument jsonDoc = QJsonDocument::fromJson(res.toUtf8());
    if(jsonDoc.isNull()||!jsonDoc.isObject()){
        showTip(tr("json 解析错误！"),false);
        return ;
    }
    QJsonObject jsonObj = jsonDoc.object();

    //处理对应的逻辑
    _handles[id](jsonObj);

    return ;
}

void logDialog::slot_socket_connect_success(bool o){
    //通过host和port连接socket至聊天服务器
    if(!o){
        showTip("连接聊天服务器失败，请检查网络...",false);
        return;
    }
    showTip("已连接至聊天服务器，正在加载中...",true);

    ///验证用户信息（uid + token）,聊天服务器到状态服务器中查询
    QJsonObject jsonObj;
    jsonObj["uid"] = _uid;
    jsonObj["token"] = _token;

    QJsonDocument doc(jsonObj);
    QByteArray jsonString = doc.toJson(QJsonDocument::Indented);

    emit TcpMgr::getInstance()->sig_send_data(ReqId::ID_CHAT_LOGIN,jsonString);

}

void logDialog::sig_login_failed(int error)
{
    QString result = QString("登陆失败，err is %1").arg(error);
    showTip(result,false);
}






























