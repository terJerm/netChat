#ifndef CHATFRIENDLISTVIEW_H
#define CHATFRIENDLISTVIEW_H

#include <QListView>
#include <QEnterEvent>
class SearchInfo;

/**************************************************************/

//   * @file:      chatfriendlistview.h
//   * @brife:     好友列表的自定义视图
//   * @date:      2025/06/10

/**************************************************************/
class chatFriendListView : public QListView
{
    Q_OBJECT
public:
    explicit chatFriendListView(QWidget *parent = nullptr);

    void mouseDoubleClickEvent(QMouseEvent *event) override;             /// 双击某一个项时槽函数

signals:
    void changeToFriendRequestView();
    void changeToFriendInfoWid(std::shared_ptr<SearchInfo>);

};

#endif // CHATFRIENDLISTVIEW_H
