#ifndef CHATUSERLISTDELEGATE_H
#define CHATUSERLISTDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>

/**************************************************************/

//   * @file:      chatuserlistdelegate.h
//   * @brife:     聊天好友列表的自定义代理
//   * @date:      2025/06/10

/**************************************************************/

class chatUserListDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit chatUserListDelegate(QObject *parent = nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

signals:
};

#endif // CHATUSERLISTDELEGATE_H






















