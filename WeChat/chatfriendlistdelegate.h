#ifndef CHATFRIENDLISTDELEGATE_H
#define CHATFRIENDLISTDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>

/**************************************************************/

//   * @file:      chatfriendlistdelegate.h
//   * @brife:     好友列表的代理 自定义实现
//   * @date:      2025/06/10

/**************************************************************/
class chatFriendListDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit chatFriendListDelegate(QObject *parent = nullptr);

    /// 重写两个虚函数实现自定义项的绘制与通知（必要的）
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

signals:
};

#endif // CHATFRIENDLISTDELEGATE_H
