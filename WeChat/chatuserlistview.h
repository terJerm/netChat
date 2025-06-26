#ifndef CHATUSERLISTVIEW_H
#define CHATUSERLISTVIEW_H

#include <QListView>
#include <QEnterEvent>
#include "global.h"

/**************************************************************/

//   * @file:      chatuserlistdelegate.h
//   * @brife:     聊天好友列表的自定义视图
//   * @date:      2025/06/10

/**************************************************************/

class chatUserListView : public QListView
{
    Q_OBJECT
public:
    explicit chatUserListView(QWidget  *parent = nullptr);

protected:
    void mouseDoubleClickEvent(QMouseEvent *event) override;             /// 双击某一个项时



private:


signals:
    void userListDoubleClicked(QString ,QList<MessageItem>,int);
};

#endif // CHATUSERLISTVIEW_H
