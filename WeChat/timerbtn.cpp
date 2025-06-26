#include "timerbtn.h"
#include <QMouseEvent>
#include <QDebug>


TimerBtn::TimerBtn(QWidget *parent):QPushButton(parent),_count(10){
    _timer = new QTimer(this);

    connect(_timer,&QTimer::timeout,[this](){
        _count--;
        if(_count <= 0){
            _timer->stop();
            _count = 10;
            this->setText("获取");
            this->setEnabled(true);
            return ;
        }
        this->setText(QString::number(_count));
    });
}

TimerBtn::~TimerBtn(){
    _timer->stop();
}

void TimerBtn::mouseReleaseEvent(QMouseEvent *e){
    if(e->button() == Qt::LeftButton){
        this->setEnabled(false);
        this->setText(QString::number(_count));
        _timer->start(1000);                     //每一秒触发一次
        emit clicked();
    }
    QPushButton::mouseReleaseEvent(e);
}
