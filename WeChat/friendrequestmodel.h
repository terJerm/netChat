#ifndef FRIENDREQUESTMODEL_H
#define FRIENDREQUESTMODEL_H

#include <QAbstractListModel>
#include <QPixmap>
#include <QList>

#include "global.h"

/// 好友申请列表自定义模型
class FriendRequestModel : public QAbstractListModel
{
    Q_OBJECT
private:
    QList<FriendRequest> m_requests;
public:
    enum RequestRoles {
        friendRequestUidRole = Qt::UserRole + 31,
        friendRequestNameRole,
        friendRequestEmailRole,
        friendRequestNickRole,
        friendRequestDescRole,
        friendRequestSexRole,
        friendRequestAvatarRole,

        friendRequestMessageRole,
        friendRequestAcceptRole
    };
    explicit FriendRequestModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;   ///重写虚函数，必须的

    void addRequest(const FriendRequest &request);          ///插入一项
    void addRequest(const FriendRequest &request,int index);///更具下标插入一项
    void setRequests(const QList<FriendRequest>& requests);
    void setItemAcceptfromUid(int uid);      /// 设置每项右侧的标识：是否添加该好友

    void initModel();

signals:
};

#endif // FRIENDREQUESTMODEL_H
