#include "chatmessagelistmodel.h"
#include <QTimer>
#include <QScrollBar>
#include <QListView>
#include <QThread>


chatMessageListMOdel::chatMessageListMOdel(QObject *parent)
    : QAbstractListModel{parent}
{}

int chatMessageListMOdel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_messages.count();
}

QVariant chatMessageListMOdel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_messages.size())
        return QVariant();

    const MessageItem &item = m_messages.at(index.row());

    switch (role) {
    case SenderIdRole: return item.senderId;
    case TextRole: return item.text;
    case AvatarRole: return item.avatar;
    case TimestampRole: return item.timestamp;
    case StatusRole: return item.status;
    case TypeRole: return item.type;
    case UidRole: return item.uid;
    case MarkingRole: return item.marking;
    default: return QVariant();
    }
}

QHash<int, QByteArray> chatMessageListMOdel::roleNames() const
{
    return {
        { SenderIdRole, "senderId" },
        { TextRole, "text" },
        { AvatarRole, "avatar" },
        { TimestampRole, "timestamp" },
        { StatusRole, "status" },
        { TypeRole, "type" },
        { UidRole, "uid"},
        { MarkingRole, "marking"},
    };
}

int chatMessageListMOdel::getSize()
{
    return m_messages.size();
}

void chatMessageListMOdel::appendMessage(const MessageItem &item)
{
    // 判断是否需要插入时间提示
    if (!m_messages.isEmpty()) {
        const MessageItem &lastMessage = m_messages.last();

        // 计算时间差（秒）
        int secondsDiff = lastMessage.timestamp.secsTo(item.timestamp);

        if (secondsDiff > 60) { // 超过5分钟
            MessageItem timeTip;
            timeTip.type = TimeItem;
            timeTip.text = item.timestamp.toString("yyyy-MM-dd HH:mm"); // 显示格式
            timeTip.timestamp = item.timestamp;

            beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
            m_messages.append(timeTip);
            endInsertRows();
        }
    }

    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
    m_messages.append(item);
    endInsertRows();
}

/// 更新某条消息的发送状态(第 row 条数据，status)
void chatMessageListMOdel::updateMessageStatus(QString marking,  MessageStatus  status){

    for (int i = m_messages.size() - 1; i >= 0; --i) {
        if (m_messages[i].marking == marking) {
            m_messages[i].status = status;
            QModelIndex index = createIndex(i, 0);
            emit dataChanged(index, index, { StatusRole }); // 通知视图只更新这一条消息
            break; // 找到后立即退出
        }
    }return;
}

void chatMessageListMOdel::setItems(const QList<MessageItem> &items)
{
    beginResetModel();
    m_messages = items;
    endResetModel();
    emit modelResetFinished();
}

QString chatMessageListMOdel::getFrontMarking()
{
    return m_messages.at(0).marking;
}

void chatMessageListMOdel::insertItem(const MessageItem &item, int index)
{
    if (index < 0 || index > m_messages.size())
        return;

    beginInsertRows(QModelIndex(), index, index);              /// 通知即将插入
    m_messages.insert(index, item);                            /// 执行插入操作
    endInsertRows();                                           /// 通知插入完成
}

void chatMessageListMOdel::insertItems(const QList<MessageItem> &items, int index)
{
    if (items.isEmpty() || index < 0 || index > m_messages.size())
        return;

    beginInsertRows(QModelIndex(), index, index + items.size() - 1);
    for (int i = 0; i < items.size(); ++i) {
        m_messages.insert(index + i, items[i]);
    }
    endInsertRows();
}

void chatMessageListMOdel::insertItems(const QList<MessageItem> &items, int index, QListView *view)
{
    // if (!view || items.isEmpty() || index < 0 || index > m_messages.size())
    //     return;

    // // 1. 获取当前第一个可见项的模型索引和它在视图中的位置
    // QModelIndex anchorIndex = view->indexAt(QPoint(0, 0));
    // if (!anchorIndex.isValid()) return;

    // // 2. 获取 anchorIndex 的全局 y 坐标位置
    // QRect anchorRect = view->visualRect(anchorIndex);
    // int anchorY = anchorRect.top();

    // // 3. 插入数据
    // beginInsertRows(QModelIndex(), index, index + items.size() - 1);
    // for (int i = 0; i < items.size(); ++i) {
    //     m_messages.insert(index + i, items[i]);
    // }
    // endInsertRows();

    // // 4. 使用 QTimer 异步恢复 scrollBar 位置（避免 UI 未更新导致失败）
    // QTimer::singleShot(0, view, [=]() {
    //     // 重新获取 anchorIndex 在新模型中的位置
    //     QRect newAnchorRect = view->visualRect(anchorIndex);

    //     // 计算插入后 anchorIndex 新的 top 坐标与旧 top 的差值
    //     int dy = newAnchorRect.top() - anchorY;

    //     // 恢复滚动条
    //     QScrollBar *bar = view->verticalScrollBar();
    //     if (bar)
    //         bar->setValue(bar->value() + dy);

    // });

    if (!view || items.isEmpty() || index < 0 || index > m_messages.size())
        return;

    // 获取第一个可见项索引（更稳定）
    QModelIndex anchorIndex = view->indexAt(view->viewport()->rect().topLeft());
    if (!anchorIndex.isValid()) return;

    // 保存该索引在视图的位置
    QRect anchorRect = view->visualRect(anchorIndex);
    int anchorY = anchorRect.top();

    // 插入消息
    beginInsertRows(QModelIndex(), index, index + items.size() - 1);
    for (int i = 0; i < items.size(); ++i) {
        m_messages.insert(index + i, items[i]);
    }
    endInsertRows();

    // 恢复滚动条位置（异步 + 更可靠）
    QTimer::singleShot(0, view->viewport(), [=]() mutable {
        QRect newAnchorRect = view->visualRect(anchorIndex);
        int dy = newAnchorRect.top() - anchorY;

        QScrollBar *bar = view->verticalScrollBar();
        if (bar) {
            int newValue = bar->value() + dy;
            bar->setValue(newValue);
            qDebug() << "Scroll restored: dy=" << dy << " newValue=" << newValue;
        }
    });
}


void chatMessageListMOdel::setLoadingToTimeStyle(QString &loadMarking, int &row)
{
    if(m_messages.at(row).marking == loadMarking){
        m_messages[row].type = MessageType::TimeItem;
        m_messages[row].text = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");

        // 通知视图这一行数据发生变化
        QModelIndex idx = index(row);
        emit dataChanged(idx, idx);
        return ;
    }qDebug()<<"loadMarking 没有找到，异常退出...";
}

const MessageItem &chatMessageListMOdel::getFrontItem()
{
    return m_messages.front();
}

void chatMessageListMOdel::initModel()
{
    beginResetModel();
    m_messages.clear();
    endResetModel();
}





