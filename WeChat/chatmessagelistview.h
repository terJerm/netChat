#ifndef CHATMESSAGELISTVIEW_H
#define CHATMESSAGELISTVIEW_H

#include <QListView>
#include <QWheelEvent>
#include <QScrollBar>

/**************************************************************/

//   * @file:      chatmessagelistview.h
//   * @brife:     聊天信息列表的自定义视图
//   * @date:      2025/06/10

/**************************************************************/

class chatMessageListView : public QListView
{
    Q_OBJECT
public:
    explicit chatMessageListView(QWidget *parent = nullptr);

    void setCanEmit(bool b);

protected:
    void wheelEvent(QWheelEvent *event) override;

signals:
    void overScrollTopTriggered();  /// 自定义信号

private:
    int accumulatedOverScroll = 0;        /// 累计向上滚动值
    const int overScrollThreshold = 400;  /// 触发阈值（可以调整）以请求更早聊天记录

    bool _can_emit = true;
};

#endif // CHATMESSAGELISTVIEW_H
