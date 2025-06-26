#include "chatfriendlistview.h"
#include "global.h"
#include "chatfriendlistmodel.h"

chatFriendListView::chatFriendListView(QWidget *parent)
    : QListView{parent}
{}

void chatFriendListView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
        qDebug()<<"there is "<<index.row()<<" item doubleClicked...";

        if(index.row() == 0){
            emit changeToFriendRequestView();
        }else{
            SearchInfo info;
            info._uid = model()->data(index,chatFriendListModel::friendUidRole).toInt();
            info._name = model()->data(index,chatFriendListModel::friendNameRole).toString();
            info._nick = model()->data(index,chatFriendListModel::friendNickRole).toString();
            info._desc = model()->data(index,chatFriendListModel::friendDescRole).toString();
            info._sex = model()->data(index,chatFriendListModel::friendSexRole).toInt();
            info._icon = model()->data(index,chatFriendListModel::friendAvatarRole).toString();

            emit changeToFriendInfoWid(std::make_shared<SearchInfo>(info));
        }
    }

    // 别忘了交给基类处理，保证默认行为正常
    QListView::mouseDoubleClickEvent(event);
}
