#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include <global.h>
#include <httpmge.h>


enum TipErr{
    TIP_SUCCESS = 0,          ///没有错误
    TIP_EMAIL_ERR = 1,        ///邮箱错误
    TIP_PWD_ERR = 2,          ///密码错误
    TIP_CONFIRM_ERR = 3,      ///确认密码错误
    TIP_VARIFY_ERR = 5,       ///验证码错误
    TIP_USER_ERR = 6          ///用户错误
};

namespace Ui {
class registerDialog;
}

/**************************************************************/

//   * @file:      registerdialog.h
//   * @brife:     注册界面
//   * @date:      2025/06/10

/**************************************************************/

class registerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit registerDialog(QWidget *parent = nullptr);
    ~registerDialog();

    void initUI();
private:
    Ui::registerDialog *ui;
    QMap<ReqId, std::function<void(const QJsonObject&)>> _handlers;  ///不同请求id 对应不同回调函数对象
    QMap<TipErr,QString> _tip_map;

public slots:
    void changeToLog();    ///跳转到登录槽函数

signals:
    void changeLogin();     ///跳转到登录信号
    void tipMapChangeSignals();
private slots:
    void tipMapChangeSlots();    ///设置按钮状态槽函数
    void on_get_captcha_clicked();     /// 获取验证码进行注册槽函数
    void slot_register_finish(ReqId id, QString res, ErrorCodes err);  ///处理注册 回包
    void on_ok_clicked();         /// 确定注册按钮槽函数

    bool checkUserValid();         /// 检查每个输入框内容是否合理
    bool checkMialboxValid();
    bool checkPassValid();
    bool checkSureValid();
    bool checkCaptchaValid();

private:
    void showTip(QString str,bool ex);
    void initHttpHandlers();      ///注册回调
    void initTipMap();
    void initLineEdit();          /// 舒适化输入框
};

#endif // REGISTERDIALOG_H
