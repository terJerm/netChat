#ifndef APPLYFRIENDWID_H
#define APPLYFRIENDWID_H

#include <QWidget>

namespace Ui {
class applyFriendWId;
}

/**************************************************************/

//   * @file:      applyfriendwid.h
//   * @brife:     添加好友界面
//   * @date:      2025/06/10

/**************************************************************/

class applyFriendWId : public QWidget
{
    Q_OBJECT

public:
    explicit applyFriendWId(QWidget *parent = nullptr);
    ~applyFriendWId();

    /// 更新按钮状态
    void updataWid(QString myname,QString tipname,int useruid);

    /// 添加好友按钮的槽函数
    void slotOkBtnClicked();

private:
    Ui::applyFriendWId *ui;

    int _user_uid;           /// 当前界面中好友的uid

signals:
    void noBtnClickedSignals();    /// 取消按钮的槽函数
    void emitFriendApplySignals();    /// 确定按钮触发后的发射的信号，给主界面通信
};

#endif // APPLYFRIENDWID_H
