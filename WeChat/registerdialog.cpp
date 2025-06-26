#include "registerdialog.h"
#include "ui_registerdialog.h"
#include "defer.h"

registerDialog::registerDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::registerDialog)
{
    ui->setupUi(this);
    ui->password->setEchoMode(QLineEdit::Password);
    ui->sure_word->setEchoMode(QLineEdit::Password);
    setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);

    connect(ui->no,&QPushButton::clicked,this,&registerDialog::changeToLog);
    connect(httpMge::getInstance().get(),&httpMge::sig_register_mod_finish,this,&registerDialog::slot_register_finish);

    ///初始化 QMap:handlers(请求id 对应 回调函数)
    initHttpHandlers();
    _tip_map.clear();
    ui->ok->setEnabled(false);
    ui->get_captcha->setEnabled(false);
    initTipMap();

    ui->account->setPlaceholderText("请输入 10 位以内用户名");
    ui->mailbox->setPlaceholderText("请输入你的邮箱地址");
    ui->password->setPlaceholderText("请输入8 ~ 15 位的密码");
    ui->sure_word->setPlaceholderText("确认你的密码");
    ui->captcha->setPlaceholderText("请输入 4 位的验证码 ");

    connect(ui->account,&QLineEdit::editingFinished,this,&registerDialog::checkUserValid);
    connect(ui->mailbox,&QLineEdit::editingFinished,this,&registerDialog::checkMialboxValid);
    connect(ui->password,&QLineEdit::editingFinished,this,&registerDialog::checkPassValid);
    connect(ui->sure_word,&QLineEdit::editingFinished,this,&registerDialog::checkSureValid);
    connect(ui->captcha,&QLineEdit::editingFinished,this,&registerDialog::checkCaptchaValid);

    connect(this,&registerDialog::tipMapChangeSignals,this,&registerDialog::tipMapChangeSlots);
}

registerDialog::~registerDialog()
{
    delete ui;
}

void registerDialog::initUI()
{
    _tip_map.clear();
    ui->account->clear();
    ui->mailbox->clear();
    ui->password->clear();
    ui->sure_word->clear();
    ui->captcha->clear();
    ui->ok->setEnabled(false);
    ui->no->setEnabled(true);
    ui->get_captcha->setEnabled(true);
}

void registerDialog::changeToLog()
{
    initLineEdit();
    initTipMap();
    ui->ok->setEnabled(false);
    ui->get_captcha->setEnabled(false);
    emit changeLogin();
}

void registerDialog::tipMapChangeSlots(){
    if(_tip_map.empty()){
        ui->tip->clear();
        ui->ok->setEnabled(true);
        return ;
    }
    ui->ok->setEnabled(false);
    showTip(_tip_map.first(),false);
}

//get 按钮按下时的槽函数
void registerDialog::on_get_captcha_clicked(){
    if(_tip_map.contains(TIP_EMAIL_ERR)){
        showTip(_tip_map[TIP_EMAIL_ERR],false);
        return ;
    }
    showTip("正在发送验证码，请等待...",true);
    //发送验证码注册
    auto email = ui->mailbox->text();
    QJsonObject json_obj;
    json_obj["email"] = email;
    httpMge::getInstance()->PostHttpReq(QUrl(gate_url_prefix+"/get_varifycode"),json_obj,
                                        ReqId::ID_GET_VARIFY_CODE,Modules::REGISRERMOD);
}

void registerDialog::slot_register_finish(ReqId id, QString res, ErrorCodes err){///请求id ，收到的数据 ，错误码
    //http单例类通过信号个槽触发的注册模块请求的回调函数
     ///qDebug()<<"jinthe";

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

    //传给对应的回调函数

    _handlers[id](jsonObj);

    return ;
}

void registerDialog::showTip(QString str,bool ex)
{
    ui->tip->setText(str);
    ui->tip->setProperty("state",ex?"normal":"error");
    repolish(ui->tip);
}

//初始化 QMap:handlers(请求id 对应 回调函数)
void registerDialog::initHttpHandlers()
{
    ///注册获取验证码回调函数的逻辑
    _handlers.insert(ReqId::ID_GET_VARIFY_CODE,[this](QJsonObject obj){
        int err = obj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"),false);
            return;
        }
        auto email = obj["email"].toString();
        qDebug()<<email;
        showTip(tr("验证码已发送到邮箱，请注意查收！"),true);

    });
    ///注册模块
    _handlers.insert(ReqId::ID_REG_USER, [this](QJsonObject jsonObj){
        ///qDebug()<<"进入";
        int error = jsonObj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("注册失败！"),false);
            return;
        }
        auto email = jsonObj["email"].toString();
        showTip(tr("用户注册成功,等待跳转..."), true);
        qDebug()<<"user uuid is : "<<jsonObj["uid"].toString();
        qDebug()<< "email is " << email ;

        ///跳转至登录界面
        QTimer::singleShot(2000, this, SLOT(changeToLog()));
    });

}

void registerDialog::initTipMap(){
    _tip_map[TIP_USER_ERR] = "用户名错误";
    _tip_map[TIP_EMAIL_ERR] = "邮箱错误";
    _tip_map[TIP_PWD_ERR] = "密码错误";
    _tip_map[TIP_CONFIRM_ERR] = "确认密码错误";
    _tip_map[TIP_VARIFY_ERR] = "验证码错误";
    ///qDebug()<<"init tipmap";
}

void registerDialog::initLineEdit()
{
    ui->account->clear();
    ui->mailbox->clear();
    ui->password->clear();
    ui->sure_word->clear();
    ui->captcha->clear();
    ///qDebug()<<"clear";
    ui->tip->clear();
}


///注册确定按钮槽函数
void registerDialog::on_ok_clicked(){
    if(!_tip_map.empty()){
        showTip(_tip_map.first(), false);
        return;
    }
    //发送一个post注册请求
    qDebug()<<"发送登录请求...";
    QJsonObject json_obj;
    json_obj["user"] = ui->account->text();
    json_obj["email"] = ui->mailbox->text();
    json_obj["passwd"] = enhancedXorEncrypt(ui->password->text(),key,salt);
    json_obj["confirm"] = enhancedXorEncrypt(ui->sure_word->text(),key ,salt);
    json_obj["varifycode"] = ui->captcha->text();
    httpMge::getInstance()->PostHttpReq(QUrl(gate_url_prefix+"/user_register"),
                                        json_obj, ReqId::ID_REG_USER,Modules::REGISRERMOD);
    showTip(tr("正在注册中..."),true);
}

bool registerDialog::checkUserValid(){
    Defer defer([this](){
        emit tipMapChangeSignals();
    });
    if(ui->account->text() == ""){
        _tip_map[TIP_USER_ERR] = "用户名不能为空";
        return false;
    }
    if(ui->account->text().length() > 10){
        _tip_map[TIP_USER_ERR] = "用户名超过10位";
        return false;
    }
    QRegularExpression re("[\u4e00-\u9fa5]");
    if(re.match(ui->account->text()).hasMatch()){
        _tip_map[TIP_USER_ERR] = "用户名只可由字母和数字组成哦";
        return false;
    }
    QRegularExpression res("^\\d+$");
    if(res.match(ui->account->text()).hasMatch()){
        _tip_map[TIP_USER_ERR] = "用户名不可由纯数字组成哦";
        return false;
    }
    _tip_map.remove(TIP_USER_ERR);
    return true;
}

bool registerDialog::checkMialboxValid(){
    Defer defer([this](){
        emit tipMapChangeSignals();
    });
    auto email = ui->mailbox->text();
    QRegularExpression regex(R"(((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+))");
    bool match = regex.match(email).hasMatch();
    if(match){
        _tip_map.remove(TIP_EMAIL_ERR);
        ui->get_captcha->setEnabled(true);
        return true;
    }else{
        _tip_map[TIP_EMAIL_ERR] = "邮箱格式不匹配";
        ui->get_captcha->setEnabled(false);
        return false;
    }
}

bool registerDialog::checkPassValid(){
    Defer defer([this](){
        emit tipMapChangeSignals();
    });
    if(ui->password->text() == ""){
        _tip_map[TIP_PWD_ERR] = "密码不能为空";
        return false;
    }if(ui->password->text().length() >15 || ui->password->text().length() < 8){
        _tip_map[TIP_PWD_ERR] = "密码长度不符合";
        return false;
    }
    _tip_map.remove(TIP_PWD_ERR);
    return true;
}

bool registerDialog::checkSureValid(){
    Defer defer([this](){
        emit tipMapChangeSignals();
    });
    if(ui->sure_word->text() == ""){
        _tip_map[TIP_CONFIRM_ERR] = "确认密码不能为空";
        return false;
    }
    if(ui->sure_word->text() != ui->password->text()){
        _tip_map[TIP_CONFIRM_ERR] = "确认密码和原密码不一致";
        return false;
    }
    _tip_map.remove(TIP_CONFIRM_ERR);
    return true;
}

bool registerDialog::checkCaptchaValid(){
    Defer defer([this](){
        emit tipMapChangeSignals();
    });
    if(ui->captcha->text() == ""){
        _tip_map[TIP_VARIFY_ERR] = "验证码不能为空";
        return false;
    }if(ui->captcha->text().length() != 4 ){
        _tip_map[TIP_VARIFY_ERR] = "验证码长度不符合";
        return false;
    }
    _tip_map.remove(TIP_VARIFY_ERR);
    return true;
}

