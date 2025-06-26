#include "chatuserlistmodel.h"
#include <QTime>
#include "usermgr.h"
#include "tcpmgr.h"

#define DEFAULT_INFORMATION "我已添加了你的好友，现在我们可以聊天了！";

/// // 提供自定义角色名
QHash<int, QByteArray> ChatUserListModel::roleNames() const{
    QHash<int, QByteArray> roles;
    roles[NameRole] = "name";
    roles[AvatarRole] = "avatar";
    roles[TimeRole] = "time";
    roles[MessageRole] = "message";
    roles[UidRole] = "uid";
    roles[ChatDataRole] = "chatdata";
    return roles;
}

ChatUserListModel::ChatUserListModel(QObject *parent)
    : QAbstractListModel{parent}
{}

///添加单条数据
void ChatUserListModel::addItem(const chatUserListItem &item){
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items.append(item);
    endInsertRows();
}

void ChatUserListModel::addItem(const chatUserListItem &item, int index)
{
    // 如果索引不合法，改为追加到末尾
    if (index < 0 || index > m_items.size()) {
        index = m_items.size();
    }
    beginInsertRows(QModelIndex(), index, index);
    m_items.insert(index, item);
    endInsertRows();
}

///重置整个数据
void ChatUserListModel::setItems(const QList<chatUserListItem> &items){
    beginResetModel();
    m_items = items;
    endResetModel();
}

int ChatUserListModel::rowCount(const QModelIndex &parent) const{
    Q_UNUSED(parent);
    return m_items.size();
}

QVariant ChatUserListModel::data(const QModelIndex &index, int role) const{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return QVariant();

    const chatUserListItem &item = m_items[index.row()];
    switch (role) {
    case NameRole: return item.name;
    case AvatarRole: return item.avatar;
    case TimeRole: return item.time;
    case MessageRole: return item.message;
    case UidRole: return item.uid;
    case ChatDataRole: {
        QVariantList chatList;
        for (const auto &message : item.chatdata) {
            QVariantMap messageMap;
            messageMap["uid"] = message.uid;
            messageMap["senderId"] = message.senderId;
            messageMap["text"] = message.text;
            messageMap["avatar"] = message.avatar;
            messageMap["timestamp"] = message.timestamp;
            messageMap["status"] = static_cast<int>(message.status);
            messageMap["type"] = static_cast<int>(message.type);
            messageMap["marking"] = message.marking;
            chatList.append(messageMap);
        }
        return chatList;
    }
    default: return QVariant();
    }
}

void ChatUserListModel::findTheItemFromUid(int uid,std::shared_ptr<FriendItem> it){
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items[i].uid == uid) {
            m_items[i].uid = (*it).uid;
            m_items[i].name = (*it).name;
            m_items[i].avatar = (*it).avatar;
            m_items[i].time = QTime::currentTime().toString("H:mm");
            m_items[i].message = "";

            // 只通知视图该行已更新
            emit dataChanged(index(i), index(i));
            qDebug()<<"这里有项，无需新家";
            return;
        }
    }
    qDebug()<<"没有此项，添加心想";
    chatUserListItem item;
    item.uid = (*it).uid ;
    item.name = (*it).name;
    item.avatar = (*it).avatar;
    item.time = QTime::currentTime().toString("H:mm");
    item.message = "";
    beginInsertRows(QModelIndex(), 0, 0);
    this->addItem(item,0);
    endInsertRows();

}

bool ChatUserListModel::findItemToInsertMessage(int &uid, MessageItem &item)
{
    for(auto& i : m_items){
        if(i.uid == uid){
            i.chatdata.append(item);
            return true;
        }
    }return false;
}

bool ChatUserListModel::SetfindItemToInsertMessage(int &uid, QString &marking)
{

    for (int row = 0; row < m_items.size(); ++row) {
        if (m_items[row].uid == uid) {
            for (auto it = m_items[row].chatdata.rbegin(); it != m_items[row].chatdata.rend(); ++it) {
                if (it->marking == marking) {
                    it->status = MessageStatus::Sent;
                    m_items[row].message = it->text;
                    m_items[row].time = QTime::currentTime().toString("H:mm");

                    // 通知视图：只更新消息文本和时间
                    QModelIndex index = createIndex(row, 0);
                    emit dataChanged(index, index, {ChatRoles::MessageRole, ChatRoles::TimeRole});
                    return true;
                }
            }
        }
    }
    return false;
}

bool ChatUserListModel::findUid(int &uid,int& index)
{
    for(int q = 0;q < m_items.count();q++){
        if(m_items[q].uid == uid){
            index = q;
            return true;
        }
    }return false;
}

MessageItem ChatUserListModel::addMessageItem(const int &index, const QString &marking, const QString &message,int type)
{
    MessageItem item;
    item.uid = m_items[index].uid;
    item.marking = marking;
    item.senderId = m_items[index].name;
    item.text = message;
    item.avatar = m_items[index].avatar;
    item.timestamp = QDateTime::currentDateTime();
    item.status = MessageStatus::Sent;
    if(type == 1){
        item.type = MessageType::NormalMessage;
    }else if(type == 2){
        item.type = MessageType::PixMap;
    }else if(type == 3){
        item.type = MessageType::File;
    }


    m_items[index].chatdata.append(item);
    m_items[index].message = message;
    m_items[index].time = QTime::currentTime().toString("H:mm");

    QModelIndex i = createIndex(index, 0);
    emit dataChanged(i, i, {ChatRoles::MessageRole, ChatRoles::TimeRole});

    return item;
}

bool ChatUserListModel::getChatMessageList(int now,int size)
{
    if(now == size ) return false;
    int selfuid = UserMgr::getInstance()->getUid();
    QJsonObject obj;
    obj["selfuid"] = selfuid;
    obj["useruid"] = m_items[now].uid;
    obj["index"] = now;
    QJsonDocument doc(obj);
    QByteArray byte = doc.toJson(QJsonDocument::Compact);
    /// 使用 tcpMgr 给服务器发送请求
    TcpMgr::getInstance()->sig_send_data(ReqId::ID_GET_CHAT_LIST_FROM_UID_REQ,byte);




    return true;
}

int ChatUserListModel::getUidFromIndex(int index)
{
    return m_items.at(index).uid;
}

int ChatUserListModel::getIndexFromUid(int uid)
{
    for(int q = 0;q<m_items.count();q++){
        if(m_items[q].uid == uid){
            return q;
        }
    }return -1;
}

QString ChatUserListModel::getIconFromIndex(int index)
{
    return m_items.at(index).avatar;
}

QString ChatUserListModel::getNameFromIndex(int index)
{
    return m_items.at(index).name;
}

void ChatUserListModel::setMessageFromIndex(int index, QList<MessageItem>list)
{
    if (index < 0 || index >= m_items.size() || list.isEmpty())
        return;

    m_items[index].chatdata = list;
    m_items[index].message = list.last().text;
    m_items[index].time = "00:00";

    // 通知视图，第 index 行的数据有变化
    QModelIndex modelIndex = this->index(index);
    emit dataChanged(modelIndex, modelIndex, {MessageRole, TimeRole});
}

void ChatUserListModel::appendMessageFromIndex(int index, QList<MessageItem> list,QString& marking)
{
    if (index < 0 || index >= m_items.size())
        return;
    if(m_items.at(index).chatdata.first().marking == marking){
        qDebug()<<"第一个marking标识 与 服务器返回的 marking 标识对应，插入位置正确...";
    }

    // 获取引用，方便操作
    QList<MessageItem>& chatList = m_items[index].chatdata;

    // 从前往后插入，保持原顺序（因为 prepend 会倒序）
    for (int i = list.size() - 1; i >= 0; --i) {
        chatList.prepend(list[i]);
    }
}

bool ChatUserListModel::getNameAndChatMessageFromUid(int &uid, QString &name, QList<MessageItem> &messages)
{
    for(const auto& it : m_items){
        if(it.uid == uid){
            name = it.name;
            messages = it.chatdata;
            return true;
        }
    }return false;
}

bool ChatUserListModel::getIconAndNameFromUid(int &uid, QString &icon, QString &name)
{
    for(const auto& it : m_items){
        if(it.uid == uid){
            icon = it.avatar;
            name = it.name;
            return true;
        }
    }
    return false;
}

chatUserListItem &ChatUserListModel::getItemFromUid(int uid)
{
    for(chatUserListItem& it : m_items){
        if(it.uid == uid){
            return it;
        }
    }return m_items.first();
}

void ChatUserListModel::initModel()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
}
























