#ifndef TIMERBTN_H
#define TIMERBTN_H

#include <QPushButton>
#include <QTimer>

/**************************************************************/

//   * @file:      timerbtn.h
//   * @brife:     发送验证码的按钮：按下后开始倒计时，结束后可再次触发
//   * @date:      2025/04/09

/**************************************************************/

class TimerBtn:public QPushButton{
    Q_OBJECT
private:
    QTimer* _timer;
    int     _count;
public:
    TimerBtn(QWidget* parent = nullptr);
    ~TimerBtn();
    void mouseReleaseEvent(QMouseEvent* e) override;
};

#endif // TIMERBTN_H
