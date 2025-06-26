#include "chatfriendlistmodel.h"

chatFriendListModel::chatFriendListModel(QObject *parent)
    : QAbstractListModel{parent},_isomve(false)
{
    // 固定项
    FriendItem topItem;
    topItem.name = "新的朋友";
    topItem.avatar = ":/res/chat_res/newfriend_1.png";
    topItem.isTopItem = true;
    m_items.append(topItem);
}

int chatFriendListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_items.count();
}

QVariant chatFriendListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return QVariant();

    const FriendItem &item = m_items[index.row()];
    switch (role) {
    case friendUidRole       :return item.uid;
    case friendNameRole      :return item.name;
    case friendEmailRole     :return item.email;
    case friendNickRole      :return item.nick;
    case friendDescRole      :return item.desc;
    case friendSexRole       :return item.sex;
    case friendAvatarRole    :return item.avatar;
    case friendIsTopItemRole :return item.isTopItem;
    default                  :return QVariant();
    }
}

QHash<int, QByteArray> chatFriendListModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[friendUidRole]       = "friendUid";
    roles[friendNameRole]      = "friendName";
    roles[friendEmailRole]     = "friendEmail";
    roles[friendNickRole]      = "friendNick";
    roles[friendDescRole]      = "friendDesc";
    roles[friendSexRole]       = "friendSex";
    roles[friendAvatarRole]    = "friendAvatar";
    roles[friendIsTopItemRole] = "friendIsTopItem";
    return roles;
}

void chatFriendListModel::addItem(const FriendItem &item)
{
    if(_isomve) m_originalDataBackup.append(item);
    else{
        beginInsertRows(QModelIndex(), m_items.count(), m_items.count());
        m_items.append(item);
        endInsertRows();
    }
}

void chatFriendListModel::addItem(const FriendItem &item, int index)
{
    /// 确保 index 在有效范围内
    int size = _isomve ? m_originalDataBackup.count() : m_items.count();
    if (index < 0 || index >= size) {
        index = size; /// 默认追加到末尾
    }
    if(_isomve) m_originalDataBackup.insert(index, item);
    else{
        beginInsertRows(QModelIndex(), index, index);
        m_items.insert(index, item);
        endInsertRows();
    }
}

void chatFriendListModel::setItems(const QList<FriendItem> &items)
{    
    // 添加一个固定项作为第一项（如“新的朋友”）
    FriendItem fixedItem;
    fixedItem.name = "新的朋友";
    fixedItem.avatar = ":/res/chat_res/newfriend_1.png"; // 你可自定义路径
    fixedItem.isTopItem = true;
    if(_isomve){
        m_originalDataBackup.clear();
        m_originalDataBackup.append(fixedItem);
        m_originalDataBackup.append(items);
        return ;
    }else{
        beginResetModel();
        m_items.clear();
        m_items.append(fixedItem);
        m_items.append(items);
        endResetModel();
    }

}

void chatFriendListModel::replaceWithTemp(const QList<FriendItem> &temp)
{
    beginResetModel();
    m_originalDataBackup = std::move(m_items);  // 高效移动
    m_items = temp;
    _isomve = true;
    endResetModel();
}

void chatFriendListModel::restoreData()
{
    beginResetModel();
    m_items = std::move(m_originalDataBackup);  // 高效恢复
    _isomve = false;
    endResetModel();
}

void chatFriendListModel::chatSearchDataQString(const QString &new_name)
{
    if(_isomve && !m_items.empty()){
        beginResetModel();
        m_items[0].name = "搜索：" + new_name;
        endResetModel();
    }
}

bool chatFriendListModel::checkUidIsExist(int uid)
{
    for(auto& item : _isomve ? m_originalDataBackup : m_items){
        if(item.uid == uid) return true;
    }
    return false;
}

FriendItem* chatFriendListModel::getItemFromUid(int uid){
    for(auto& item : _isomve ? m_originalDataBackup : m_items){
        if(item.uid == uid) return &item;
    }
    return nullptr; // 如果未找到返回 nullptr
}

void chatFriendListModel::initModel()
{
    beginResetModel();
    m_items.clear();
    m_originalDataBackup.clear();
    endResetModel();
}






















