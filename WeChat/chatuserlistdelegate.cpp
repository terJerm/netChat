#include "chatuserlistdelegate.h"
#include <QPainter>
#include <QColor>
#include <QListView>
#include <QScrollBar>

chatUserListDelegate::chatUserListDelegate(QObject *parent)
    : QStyledItemDelegate{parent}
{}

void chatUserListDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();

    QRect rect = option.rect;
    rect.setWidth(option.widget->width()); // 强制用listView的可视宽度


    // 绘制悬停/选中背景
    if (option.state & QStyle::State_MouseOver) {
        painter->fillRect(rect, QColor(230, 230, 230)); // 鼠标悬停时浅灰背景
    } else if (option.state & QStyle::State_Selected) {
        painter->fillRect(rect, QColor(200, 200, 200)); // 如果还想兼容选中状态，也可以加
    }

    // 获取数据
    QString name = index.data(Qt::UserRole + 1).toString();     // name
    QString avatar = index.data(Qt::UserRole + 2).toString(); // avatar
    QString time = index.data(Qt::UserRole + 3).toString();     // time
    QString message = index.data(Qt::UserRole + 4).toString();  // message

    int margin = 10;
    int spacing = 5;
    int avatarSize = rect.height() - 2 * margin;

    QRect avatarRect(rect.left() + margin, rect.top() + margin, avatarSize, avatarSize);
    // 绘制头像
    QPixmap avatarMap;
    if (!avatar.isEmpty()) {
        avatarMap.load(avatar);
    }else{
        avatarMap = QPixmap(":/res/chat_res/self_1.png");
    }
    if (!avatarMap.isNull()) {
        painter->drawPixmap(avatarRect, avatarMap.scaled(avatarRect.size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    }

    // 名称 + 时间 在顶部
    QFont nameFont("宋体", 11, QFont::Bold);
    QFont timeFont("宋体", 9);
    QFont messageFont("宋体", 10);

    int textLeft = avatarRect.right() + spacing;
    int textRight = rect.right() - margin;

    QRect nameRect(textLeft, rect.top() + margin, textRight - textLeft - 40, 20);
    QRect timeRect(textRight - 60, rect.top() + margin, 50, 20);

    painter->setFont(nameFont);
    painter->setPen(QColor::fromRgb(62,214,209,255));
    painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignVCenter, painter->fontMetrics().elidedText(name, Qt::ElideRight, nameRect.width()));

    painter->setFont(timeFont);
    painter->setPen(Qt::gray);
    painter->drawText(timeRect, Qt::AlignRight | Qt::AlignVCenter, painter->fontMetrics().elidedText(time, Qt::ElideRight, timeRect.width()));

    // 第二行：消息内容
    QRect messageRect(textLeft, rect.top() + margin + 22, textRight - textLeft-20, 20);
    painter->setFont(messageFont);
    painter->setPen(Qt::gray);
    painter->drawText(messageRect, Qt::AlignLeft | Qt::AlignVCenter, painter->fontMetrics().elidedText(message, Qt::ElideRight, messageRect.width()));

    painter->restore();

}

QSize chatUserListDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    Q_UNUSED(option);
    Q_UNUSED(index);
    return QSize(200, 60); // 每项高度 60px
}





















