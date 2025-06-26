#include "friendrequestmodel.h"

FriendRequestModel::FriendRequestModel(QObject *parent)
    : QAbstractListModel{parent}
{}

int FriendRequestModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_requests.count();
}

QVariant FriendRequestModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_requests.size())
        return {};
    const auto &req = m_requests[index.row()];
    switch (role) {
    case friendRequestAvatarRole   : return req.avatar;
    case friendRequestNameRole     : return req.name;
    case friendRequestMessageRole  : return req.message;
    case friendRequestAcceptRole   : return req.accepted;
    case friendRequestUidRole      : return req.uid;
    case friendRequestEmailRole    : return req.email;
    case friendRequestNickRole     : return req.nick;
    case friendRequestDescRole     : return req.desc;
    case friendRequestSexRole      : return req.sex;
    default: return {};
    }
}

bool FriendRequestModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_requests.size())
        return false;

    auto &req = m_requests[index.row()];
    if (role == friendRequestAcceptRole && value.canConvert<bool>()) {
        req.accepted = value.toBool();
        emit dataChanged(index, index, {role});
        return true;
    }

    return false;
}

Qt::ItemFlags FriendRequestModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QHash<int, QByteArray> FriendRequestModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[friendRequestAvatarRole]     = "friendRequestAvatar";
    roles[friendRequestNameRole]       = "friendRequestName";
    roles[friendRequestMessageRole]    = "friendRequestMessage";
    roles[friendRequestAcceptRole]     = "friendRequestAccepted";
    roles[friendRequestUidRole]        = "friendRequestUid";
    roles[friendRequestEmailRole]      = "friendRequestEmail";
    roles[friendRequestNickRole]       = "friendRequestNick";
    roles[friendRequestDescRole]       = "friendRequestDesc";
    roles[friendRequestSexRole]        = "friendRequestSex";


    return roles;
}

void FriendRequestModel::addRequest(const FriendRequest &request)
{
    beginInsertRows(QModelIndex(), m_requests.size(), m_requests.size());
    m_requests.append(request);
    endInsertRows();
}

void FriendRequestModel::addRequest(const FriendRequest &request, int index)
{
    if (index < 0 || index > m_requests.size())
        index = m_requests.size(); /// 如果索引越界，则追加到末尾

    beginInsertRows(QModelIndex(), index, index);
    m_requests.insert(index, request);
    endInsertRows();
}

void FriendRequestModel::setRequests(const QList<FriendRequest> &requests)
{
    beginInsertRows(QModelIndex(), m_requests.size(), m_requests.size());
    m_requests = requests;
    endInsertRows();
}

void FriendRequestModel::setItemAcceptfromUid(int uid)
{
    for (int i = 0; i < m_requests.size(); ++i) {
        if (m_requests[i].uid == uid) {
            m_requests[i].accepted = true;
            /// 通知视图该行已更新
            emit dataChanged(index(i), index(i));
            return;
        }
    }
}

void FriendRequestModel::initModel()
{
    beginResetModel();
    m_requests.clear();
    endResetModel();
}
