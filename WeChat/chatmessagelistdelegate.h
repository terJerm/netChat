#ifndef CHATMESSAGELISTDELEGATE_H
#define CHATMESSAGELISTDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QTimer>

/**************************************************************/

//   * @file:      chatmessagelistdelegate.h
//   * @brife:     聊天界面列表的自定义代理
//   * @date:      2025/06/10

/**************************************************************/

class chatMessageListDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit chatMessageListDelegate(QObject *parent = nullptr);

    /// 重写函数
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;           /// 绘制消息
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;       /// 计算每条消息的高度


    void drawStar(QPainter *painter) const;   /// 绘制一条消息成功发送前的 旋转五角星

protected:
    bool editorEvent(QEvent *event, QAbstractItemModel *model,
                     const QStyleOptionViewItem &option, const QModelIndex &index) override;

private:
    QTimer *m_timer;
    mutable int m_rotateAngle = 0;

    int avatarSize;
    int maxTextWidth;
    int minTextWidth;

signals:
    void needUpdate();

    void downloadClicked(const QModelIndex &index);        /// 文件下载按钮的槽函数
    void openClicked(const QModelIndex &index);            /// 文件打开按钮的槽函数
};

#endif // CHATMESSAGELISTDELEGATE_H
