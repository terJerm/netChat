#include "findpasswddialog.h"
#include "ui_findpasswddialog.h"
#include "httpmge.h"
#include <QTimer>

findPasswdDialog::findPasswdDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::findPasswdDialog)
{
    ui->setupUi(this);
    setWindowFlags(Qt::CustomizeWindowHint|Qt::FramelessWindowHint);

    ui->userLine->setPlaceholderText("请输入 10 位以内用户名");
    ui->mialline->setPlaceholderText("请输入你的邮箱地址");
    ui->newline->setPlaceholderText("请输入8 ~ 15 位的密码");
    ui->captline->setPlaceholderText("请输入 4 位的验证码 ");
    initHandles();

    connect(httpMge::getInstance().get(),&httpMge::sig_findpasswd_mod_finish,this,&findPasswdDialog::slot_findpasswd_finish);
}

findPasswdDialog::~findPasswdDialog()
{
    delete ui;
}

void findPasswdDialog::initUI()
{
    ui->userLine->setText("");
    ui->mialline->clear();
    ui->captline->clear();
    ui->newline->clear();
    showTip("",true);
    _old_mialbox = "";
    ui->no->setEnabled(true);
    ui->get->setEnabled(true);
    ui->sure->setEnabled(false);
}




void findPasswdDialog::on_sure_clicked(){
    if(ui->userLine->text().length() < 1 ||ui->userLine->text().length() > 10){
        showTip("用户名格式不正确",false); return ;
    }if(ui->mialline->text() != _old_mialbox){
        showTip("邮箱地址发送改动，请重新发送验证码",false); return ;
    }if(ui->captline->text().length() != 4){
        showTip("验证码格式不正确",false); return ;
    }if(ui->newline->text().length() < 8 || ui->newline->text().length() > 15){
        showTip("密码格式不正确",false); return ;
    }

    showTip("正在重置密码，请稍等...",true);
    ///发送请求
    QJsonObject json_obj;
    json_obj["user"] = ui->userLine->text();
    json_obj["email"] = ui->mialline->text();
    json_obj["passwd"] = enhancedXorEncrypt(ui->newline->text(),key,salt);
    json_obj["varifycode"] = ui->captline->text();
    httpMge::getInstance()->PostHttpReq(QUrl(gate_url_prefix+"/reset_pwd"),json_obj,
                                        ReqId::ID_RESET_PED,Modules::FINDPASSWOEDMOD);
}


void findPasswdDialog::on_no_clicked()
{
    ui->userLine->clear();
    ui->mialline->clear();
    ui->captline->clear();
    ui->newline->clear();
    ui->tip->clear();
    emit changeToLog();
}


void findPasswdDialog::on_get_clicked(){
    if(ui->mialline->text() == ""){
        showTip("邮箱地址不能为空",false); return ;
    }
    auto email = ui->mialline->text();
    QRegularExpression regex(R"(((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+))");
    bool match = regex.match(email).hasMatch();
    if(!match){
        showTip("邮箱格式不匹配",false); return ;
    }
    showTip("正在发送验证码，请等待...",true);
    _old_mialbox = ui->mialline->text();

    ///发送验证码请求找回密码
    QJsonObject json_obj;
    json_obj["email"] = email;
    httpMge::getInstance()->PostHttpReq(QUrl(gate_url_prefix+"/get_varifycode"),json_obj,
                                        ReqId::ID_GET_VARIFY_CODE,Modules::FINDPASSWOEDMOD);

}

void findPasswdDialog::slot_findpasswd_finish(ReqId id, QString res, ErrorCodes err){
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

void findPasswdDialog::changeToLoginSlot(){
    on_no_clicked();
}

void findPasswdDialog::showTip(QString str, bool ex)
{
    ui->tip->setText(str);
    ui->tip->setProperty("state",ex?"normal":"error");
    repolish(ui->tip);
}

void findPasswdDialog::initHandles(){
    ///注册获取验证码回调函数的逻辑
    _handles.insert(ReqId::ID_GET_VARIFY_CODE,[this](QJsonObject obj){
        int err = obj["error"].toInt();
        if(err != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"),false);
            return;
        }
        auto email = obj["email"].toString();
        qDebug()<<email;
        showTip(tr("验证码已发送到邮箱，请注意查收！"),true);
    });
    //重置密码逻辑
    _handles.insert(ReqId::ID_RESET_PED,[this](QJsonObject obj){
        int error = obj["error"].toInt();
        if(error != ErrorCodes::SUCCESS){
            showTip(tr("参数错误"),false);
            return;
        }
        auto email = obj["email"].toString();
        showTip(tr("密码重置成功,等待跳转..."), true);
        qDebug()<< "email is : " << email ;

        ///跳转至登录界面
        QTimer::singleShot(2000, this, SLOT(changeToLoginSlot()));
    });
}

