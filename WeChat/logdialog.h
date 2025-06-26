#ifndef LOGDIALOG_H
#define LOGDIALOG_H

#include <QDialog>
#include <global.h>

namespace Ui {
class logDialog;
}

/**************************************************************/

//   * @file:      logdialog.h
//   * @brife:     登录界面
//   * @date:      2025/03/23

/**************************************************************/


class logDialog : public QDialog
{
    Q_OBJECT

public:
    explicit logDialog(QWidget *parent = nullptr);
    ~logDialog();

    void initUI();

private:
    Ui::logDialog *ui;
    bool _acount_valid,_passwd_valid;
    int _uid;
    QString _token;
    QMap<ReqId,std::function<void(const QJsonObject&)>>_handles;

    void showTip(QString str,bool ex);
    void initHandles();        /// 注册回调

public slots:
    void switchRegister();      /// 跳转到注册界面槽函数
    void switchForget();        /// 跳转到忘记密码模块槽函数
    void logSlot();             /// 点击登录槽函数
    void logHttpFinishSlot(ReqId id, QString res, ErrorCodes err);   ///登录回调函数
    void slot_socket_connect_success(bool o);   ///登录成功后（账号密码正确）尝试连接至聊天服务器
    void sig_login_failed(int error);

signals:
    void switchRgt();
    void switchForgetSignal();
    void sig_connect_tcp_server(ServerInfo si);
    void switchChatDialog();             ///给mainwindow发送信号跳转

};



#endif // LOGDIALOG_H
