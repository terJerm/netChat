#include "chatuserlistview.h"
#include "chatuserlistmodel.h"

chatUserListView::chatUserListView(QWidget  *parent):QListView(parent)
{


}

void chatUserListView::mouseDoubleClickEvent(QMouseEvent *event)
{
    QModelIndex index = indexAt(event->pos());
    if (index.isValid()) {
        qDebug()<<"there is "<<index.row()<<" item doubleClicked...";

        QString name = model()->data(index, ChatUserListModel::NameRole).toString();
        int uid = model()->data(index,ChatUserListModel::UidRole).toInt();
        qDebug()<<"uid is : "<<uid;
        QVariantList chatDataList = model()->data(index, ChatUserListModel::ChatDataRole).toList();
        qDebug()<<"size is : "<<chatDataList.size();
        QList<MessageItem> mess;

        for (const QVariant &chatDataVariant : chatDataList) {
            QVariantMap messageMap = chatDataVariant.toMap();
            MessageItem message;
            message.uid = messageMap.value("uid").toInt();
            message.senderId = messageMap.value("senderId").toString();
            message.text = messageMap.value("text").toString();
            message.avatar = messageMap.value("avatar").toString();
            message.timestamp = messageMap.value("timestamp").toDateTime();
            message.marking = messageMap.value("marking").toString();
            message.status = static_cast<MessageStatus>(messageMap.value("status").toInt());
            message.type = static_cast<MessageType>(messageMap.value("type").toInt());

            mess.append(message);
        }
        qDebug() << "Double-clicked user:" << name;
        qDebug() << "Loaded messages:" << mess.size();

        emit userListDoubleClicked(name, mess,uid);
    }

    // 别忘了交给基类处理，保证默认行为正常
    QListView::mouseDoubleClickEvent(event);
}


