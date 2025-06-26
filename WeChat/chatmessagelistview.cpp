#include "chatmessagelistview.h"

chatMessageListView::chatMessageListView(QWidget *parent)
    : QListView{parent}
{}

void chatMessageListView::setCanEmit(bool b)          /// 在回包中设置为true ,可以再次触发
{
    _can_emit = b;
}

void chatMessageListView::wheelEvent(QWheelEvent *event)
{
    if(!_can_emit){
        QListView::wheelEvent(event);
        return ;
    }

    bool atTop = (verticalScrollBar()->value() == verticalScrollBar()->minimum());
    int deltaY = event->angleDelta().y();

    if (atTop && deltaY > 0) {
        accumulatedOverScroll += deltaY;

        if (accumulatedOverScroll >= overScrollThreshold) {
            _can_emit = false;
            emit overScrollTopTriggered();
            accumulatedOverScroll = 0;  // 重置累计值
        }
    } else {
        // 如果不在顶部或是反方向滚动，重置累计值
        accumulatedOverScroll = 0;
    }

    // 保持原有滚动行为
    QListView::wheelEvent(event);
}
