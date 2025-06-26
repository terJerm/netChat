#ifndef FINDPASSWDDIALOG_H
#define FINDPASSWDDIALOG_H

#include <QDialog>
#include "global.h"

namespace Ui {
class findPasswdDialog;
}

/**************************************************************/

//   * @file:      findpasswddialog.h
//   * @brife:     找回密码界面
//   * @date:      2025/06/10

/**************************************************************/

class findPasswdDialog : public QDialog
{
    Q_OBJECT

public:
    explicit findPasswdDialog(QWidget *parent = nullptr);
    ~findPasswdDialog();

    void initUI();

private slots:
    void on_sure_clicked();
    void on_no_clicked();
    void on_get_clicked();
    void slot_findpasswd_finish(ReqId id, QString res, ErrorCodes err);
    void changeToLoginSlot();      ///跳转到登录界面的槽函数，发送下面那个信号
signals:
    void changeToLog();            ///跳转到登录界面的信号


private:
    Ui::findPasswdDialog *ui;
    QString _old_mialbox;
    QMap<ReqId,std::function<void (const QJsonObject&)>> _handles;  /// 回调map，通过服务器返回值调用相应函数

    void showTip(QString str,bool ex);
    void initHandles();   ///注册回调函数
};

#endif // FINDPASSWDDIALOG_H
