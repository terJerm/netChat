#ifndef CHATMESSAGELISTMODEL_H
#define CHATMESSAGELISTMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QListView>
#include "global.h"

/**************************************************************/

//   * @file:      chatmessagelistmodel.h
//   * @brife:     聊天界面列表的自定义模型
//   * @date:      2025/06/10

/**************************************************************/
class chatMessageListMOdel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit chatMessageListMOdel(QObject *parent = nullptr);

    // 自定义角色
    enum MessageRoles {
        SenderIdRole = Qt::UserRole + 11,
        TextRole,
        AvatarRole,
        TimestampRole,
        StatusRole,
        TypeRole,         // 消息类型（普通消息 or 时间提示）
        MarkingRole,
        UidRole,
    };

    /// 基本函数重写
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int getSize();   /// 获取模型中数据个数


    /// 业务逻辑接口
    void appendMessage(const MessageItem &item);                   // 添加一条消息
    void updateMessageStatus(QString marking, MessageStatus  status);      // 更新某条消息的发送状态(第 row 条数据，status)
    void setItems(const QList<MessageItem>& items);                // 重置整个数据列表
    QString getFrontMarking();   ///获取第一条数据的 marking,用于加载更早的聊天记录
    void insertItem(const MessageItem& item,int index);       /// 往数据列表中添加项
    void insertItems(const QList<MessageItem>& items, int index);
    void insertItems(const QList<MessageItem> &items, int index, QListView* view);

    void setLoadingToTimeStyle(QString& loadMarking,int& row); ///加载更早时期的消息时，现有一个"loading"标识，在收到
                                        /// 服务器回包后，把他改为 时间 标识

    const MessageItem& getFrontItem();

    void initModel();

private:
    QList<MessageItem> m_messages;        ///聊天数据

signals:
    void modelResetFinished();
};

#endif // CHATMESSAGELISTMODEL_H
