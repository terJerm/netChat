#ifndef CHATUSERLISTMODEL_H
#define CHATUSERLISTMODEL_H

#include <QAbstractListModel>
#include "global.h"
#include <QList>

/**************************************************************/

//   * @file:      chatuserlistdelegate.h
//   * @brife:     聊天好友列表的自定义模型
//   * @date:      2025/06/10

/**************************************************************/

class ChatUserListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum ChatRoles{                      ///定义了 4 个字段
        NameRole = Qt::UserRole + 1,
        AvatarRole,
        TimeRole,
        MessageRole,
        UidRole,
        ChatDataRole,
    };

    explicit ChatUserListModel(QObject *parent = nullptr);
    void addItem(const chatUserListItem& item);              ///添加单条数据
    void addItem(const chatUserListItem& item,int index);
    void setItems(const QList<chatUserListItem>& items);     ///重置整个数据列表

    QHash<int, QByteArray> roleNames() const override;       ///// 提供自定义角色名
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void findTheItemFromUid(int uid,std::shared_ptr<FriendItem> item);  /// 通过uid 添加项：如果数据中有该条数据则更新，如果没有则添加此条新数据

    bool findItemToInsertMessage(int& uid,MessageItem& item);  ///通过uid插入聊天记录
    bool SetfindItemToInsertMessage(int& uid,QString& marking);
    bool findUid(int& uid,int& index); /// 查找数据中 uid 是否存在
    MessageItem addMessageItem(const int& index,const QString& marking,const QString& message,int type);
    bool getChatMessageList(int nowIndex,int size);
    int getUidFromIndex(int index);
    int getIndexFromUid(int uid);
    QString getIconFromIndex(int index);
    QString getNameFromIndex(int index);
    void setMessageFromIndex(int index,QList<MessageItem>);
    void appendMessageFromIndex(int index,QList<MessageItem>,QString& marking);
    bool getNameAndChatMessageFromUid(int& uid,QString& name,QList<MessageItem>& messages);
    bool getIconAndNameFromUid(int& uid,QString& icon,QString& name);
    chatUserListItem& getItemFromUid(int uid);

    void initModel();
private:
    QList<chatUserListItem> m_items;      ///模型的数据

signals:
};

#endif // CHATUSERLISTMODEL_H




















