#ifndef CHATFRIENDLISTMODEL_H
#define CHATFRIENDLISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include "global.h"

/**************************************************************/

//   * @file:      chatfriendlistmodel.h
//   * @brife:     好友列表的模型自定义实现
//   * @date:      2025/06/10

/**************************************************************/
class chatFriendListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum FriendRoles {
        friendUidRole = Qt::UserRole + 21,
        friendNameRole,
        friendEmailRole,
        friendNickRole,
        friendDescRole,
        friendSexRole,
        friendAvatarRole,
        friendIsTopItemRole,
    };

    explicit chatFriendListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addItem(const FriendItem& item);                    // 添加单条数据
    void addItem(const FriendItem& item ,int index);
    void setItems(const QList<FriendItem>& items);           // 批量重置数据

    void replaceWithTemp(const QList<FriendItem>& temp);     // 临时替换数据
    void restoreData();                                      // 高效恢复数据
    void chatSearchDataQString(const QString& );

    bool checkUidIsExist(int);
    FriendItem* getItemFromUid(int uid);          /// 通过uid 查找数据列表中的项

    void initModel();

private:
    QList<FriendItem> m_items;
    QList<FriendItem> m_originalDataBackup;

    bool _isomve;



signals:
};

#endif // CHATFRIENDLISTMODEL_H
