#ifndef FRIENDREQUESTDELEGATE_H
#define FRIENDREQUESTDELEGATE_H

#include <QStyledItemDelegate>
#include <QPushButton>
#include <QPainter>

/// 好友申请列表自定义代理
class FriendRequestDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit FriendRequestDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    bool editorEvent(QEvent *event, QAbstractItemModel *model,
                     const QStyleOptionViewItem &option, const QModelIndex &index) override;

signals:
    void acceptFriendApply(int uid);
private slots:
    void slot_acceptFriendApply(int uid);
};

#endif // FRIENDREQUESTDELEGATE_H
